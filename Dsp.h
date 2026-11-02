#pragma once
// BvM DSP - C++ pur (aucune dépendance JUCE), aucune allocation en temps réel.
#include "Params.h"
#include <cstring>

namespace bvm {

constexpr float kPi = 3.14159265358979f;
inline float dbToLin(float d) { return std::pow(10.f, d * 0.05f); }
inline float linToDb(float l) { return 20.f * std::log10(std::max(l, 1e-9f)); }
inline float clampf(float v, float a, float b) { return std::min(std::max(v, a), b); }
// coefficient de lissage "un pôle" pour une constante de temps en ms
inline float coefMs(float ms, double fs) { return 1.f - std::exp(-1.f / (std::max(ms, 0.01f) * 0.001f * (float) fs)); }

// ------------------------------------------------------------------ Biquad (RBJ)
struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float process(float x) { float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
    void set(float B0, float B1, float B2, float A0, float A1, float A2) {
        float i = 1.f / A0; b0 = B0 * i; b1 = B1 * i; b2 = B2 * i; a1 = A1 * i; a2 = A2 * i;
    }
    static float w(double fs, float f) { return 2.f * kPi * std::min(f, 0.45f * (float) fs) / (float) fs; }
    void peak(double fs, float f, float q, float gDb) {
        float A = std::pow(10.f, gDb / 40.f), ww = w(fs, f), c = std::cos(ww), al = std::sin(ww) / (2.f * q);
        set(1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowShelf(double fs, float f, float gDb) {
        float A = std::pow(10.f, gDb / 40.f), ww = w(fs, f), c = std::cos(ww), s = std::sin(ww);
        float be = 2.f * std::sqrt(A) * (s / (2.f * 0.7071f));
        set(A * ((A + 1) - (A - 1) * c + be), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - be),
            (A + 1) + (A - 1) * c + be, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - be);
    }
    void highShelf(double fs, float f, float gDb) {
        float A = std::pow(10.f, gDb / 40.f), ww = w(fs, f), c = std::cos(ww), s = std::sin(ww);
        float be = 2.f * std::sqrt(A) * (s / (2.f * 0.7071f));
        set(A * ((A + 1) + (A - 1) * c + be), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - be),
            (A + 1) - (A - 1) * c + be, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - be);
    }
    void highPass(double fs, float f, float q) {
        float ww = w(fs, f), c = std::cos(ww), al = std::sin(ww) / (2.f * q);
        set((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void bandPass(double fs, float f, float q) {
        float ww = w(fs, f), c = std::cos(ww), al = std::sin(ww) / (2.f * q);
        set(al, 0, -al, 1 + al, -2 * c, 1 - al);
    }
};

struct FixedDelay {
    std::vector<float> b; int mask = 0, wp = 0, len = 0;
    void prepare(int L) { len = L; int n = 1; while (n < L + 2) n <<= 1; b.assign((size_t) n, 0.f); mask = n - 1; wp = 0; }
    inline float process(float x) { b[(size_t) wp] = x; float y = b[(size_t) ((wp - len) & mask)]; wp = (wp + 1) & mask; return y; }
};

// ------------------------------------------------------------------ Détection de hauteur (YIN décimé à ~12 kHz)
struct PitchDetector {
    static constexpr int W = 256, MAXT = 176, MINT = 14, RING = 512, HOP = 64;
    float ring[RING] = {}; int wp = 0, cnt = 0, accN = 0, decim = 4; float acc = 0, fsDec = 12000.f, f0 = 0.f;
    unsigned long long analysisCount = 0; // diagnostic counter; no allocation or locking
    void prepare(double fs) { decim = std::max(1, (int) std::lround(fs / 12000.0)); fsDec = (float) (fs / decim); std::memset(ring, 0, sizeof(ring)); wp = cnt = accN = 0; acc = 0; f0 = 0; analysisCount = 0; }
    bool push(float x) {
        acc += x; if (++accN < decim) return false;
        ring[wp] = acc / (float) decim; acc = 0; accN = 0; wp = (wp + 1) & (RING - 1);
        if (++cnt < HOP) return false;
        cnt = 0; analyse(); return true;
    }
    void analyse() {
        ++analysisCount;
        float buf[W + MAXT];
        for (int i = 0; i < W + MAXT; ++i) buf[i] = ring[(wp - (W + MAXT) + i) & (RING - 1)];
        float e = 0; for (int i = 0; i < W; ++i) e += buf[i] * buf[i];
        if (e / W < 1e-6f) { f0 = 0; return; }
        float d[MAXT + 1]; d[0] = 1.f; float run = 0;
        for (int t = 1; t <= MAXT; ++t) {
            float s = 0; for (int j = 0; j < W; ++j) { float u = buf[j] - buf[j + t]; s += u * u; }
            run += s; d[t] = run > 0 ? s * (float) t / run : 1.f;
        }
        int tau = -1;
        for (int t = MINT; t < MAXT; ++t) if (d[t] < 0.15f) { while (t + 1 < MAXT && d[t + 1] < d[t]) ++t; tau = t; break; }
        if (tau < 0) { f0 = 0; return; }
        float better = (float) tau;
        if (tau > 1 && tau < MAXT) {
            float s0 = d[tau - 1], s1 = d[tau], s2 = d[tau + 1], den = 2.f * (2.f * s1 - s2 - s0);
            if (std::fabs(den) > 1e-9f) better = (float) tau + (s2 - s0) / den;
        }
        f0 = fsDec / better;
    }
};

// ------------------------------------------------------------------ Pitch shifter (2 têtes, fondu lissé)
struct Shifter {
    std::vector<float> buf; int mask = 0, w = 0; float phase = 0.f, win = 1920.f;
    void prepare(double fs) { win = (float) (0.04 * fs); int n = 1; while (n < (int) win + 8) n <<= 1; buf.assign((size_t) n, 0.f); mask = n - 1; w = 0; phase = 0; }
    inline float rd(float d) const {
        float p = (float) w - d; int i = (int) std::floor(p); float f = p - (float) i;
        return buf[(size_t) (i & mask)] * (1.f - f) + buf[(size_t) ((i + 1) & mask)] * f;
    }
    inline float process(float x, float ratio) {
        buf[(size_t) w] = x;
        phase += (1.f - ratio) / win; phase -= std::floor(phase);
        float p2 = phase + 0.5f; p2 -= std::floor(p2);
        float t = 1.f - std::fabs(2.f * phase - 1.f), g1 = t * t * (3.f - 2.f * t), g2 = 1.f - g1;
        float y = rd(phase * win) * g1 + rd(p2 * win) * g2;
        w = (w + 1) & mask; return y;
    }
};

// ------------------------------------------------------------------ Gammes
inline bool inScale(int sc, int pc) {
    static const bool T[4][12] = {{1,0,1,0,1,1,0,1,0,1,0,1}, {1,0,1,1,0,1,0,1,1,0,1,0}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,0,0,1,0,1,0,1,0,0,1,0}};
    return T[std::min(std::max(sc, 0), 3)][pc];
}
inline int pcOf(int n, int key) { return ((n - key) % 12 + 12) % 12; }
inline float snapNote(float m, int key, int sc) {
    int r = (int) std::lround(m); float best = m, bd = 1e9f;
    for (int n = r - 6; n <= r + 6; ++n) if (inScale(sc, pcOf(n, key))) { float d = std::fabs((float) n - m); if (d < bd) { bd = d; best = (float) n; } }
    return best;
}
inline float stepNote(float n, int steps, int key, int sc) {
    int note = (int) std::lround(n), dir = steps > 0 ? 1 : -1, c = std::abs(steps);
    while (c > 0) { note += dir; if (inScale(sc, pcOf(note, key))) --c; }
    return (float) note;
}

// ------------------------------------------------------------------ Chaîne voix (1 canal)
struct Channel {
    double fs = 48000.0;
    Biquad hpf1, hpf2, boomDet, boomCut, dsDet, dsCut, eqL, eqB, eqP, eqA, softP, softS, anLow, anHigh;
    PitchDetector pd; Shifter shMain, sh1, sh2; FixedDelay dry;
    // gate
    float gateEnv = 0, gateGain = 1, gateThrLin = 0.002f; int gateHold = 0;
    // anti-boom
    float boomLo = 0, boomFull = 0, boomDb = 0;
    // de-esser
    float dsEnv = 0, dsRed = 0;
    // compresseur
    float compEnv = 0, compMakeup = 0, compThr = -18.f, compRatio = 3.f;
    // IA (analyse adaptative)
    float fullPow = 0, lowPow = 0, highPow = 0, aiGainDb = 0, aiGainTarget = 1, aiGainLin = 1, aiTilt = 0, aiMud = 0, f0Avg = 0;
    // pitch
    bool voiced = false;
    bool pitchEnabled = false, harmonyEnabled = false; // cached once per block: avoid running YIN when unused
    float midiRaw = 60, midiSnap = 60, hm1 = 60, hm2 = 60, shiftSemi = 0, hs1 = 0, hs2 = 0, voicedGain = 0;
    // coefficients
    float gEnvR = 0, gAtk = 0, gRel = 0, dsA = 0, dsR = 0, envC = 0, anC = 0, hC = 0, vgC = 0, ptC = 0, cAtkC = 0, cRelC = 0;
    // Protection de sortie : filtre DC + limiteur de crête à attaque instantanée.
    float dcX1 = 0.f, dcY1 = 0.f;

    int latency() const { return (int) (shMain.win * 0.5f); }

    void prepare(double sr) {
        fs = sr;
        for (Biquad* b : {&hpf1, &hpf2, &boomDet, &boomCut, &dsDet, &dsCut, &eqL, &eqB, &eqP, &eqA, &softP, &softS, &anLow, &anHigh}) *b = Biquad();
        pd.prepare(fs); shMain.prepare(fs); sh1.prepare(fs); sh2.prepare(fs); dry.prepare(latency());
        boomDet.bandPass(fs, 180.f, 1.f); anLow.bandPass(fs, 250.f, 0.5f); anHigh.highPass(fs, 5000.f, 0.7071f);
        gEnvR = coefMs(30, fs); gAtk = coefMs(2, fs); dsA = coefMs(0.5f, fs); dsR = coefMs(30, fs);
        envC = coefMs(20, fs); anC = coefMs(400, fs); hC = coefMs(10, fs); vgC = coefMs(40, fs);
        gateEnv = 0; gateGain = 1; gateHold = 0; boomLo = boomFull = boomDb = 0; dsEnv = dsRed = 0; compEnv = 0;
        fullPow = lowPow = highPow = 0; aiGainDb = aiTilt = aiMud = 0; aiGainTarget = aiGainLin = 1; f0Avg = 0;
        voiced = false; pitchEnabled = harmonyEnabled = false; shiftSemi = hs1 = hs2 = voicedGain = 0;
        dcX1 = dcY1 = 0.f;
    }

    void processBlock(float* d, int n, const Settings& s) {
        // Pitch detection is the most CPU-intensive stage. Do not run it unless
        // pitch correction or at least one harmony voice is actually requested.
        pitchEnabled = s.p[P::pitchOn] > 0.5f;
        harmonyEnabled = s.p[P::hMix] > 0.001f &&
            (std::lround(s.p[P::h1]) != 0 || std::lround(s.p[P::h2]) != 0);
        if (!pitchEnabled && !harmonyEnabled) voiced = false;
        const int SUB = 64;
        for (int i0 = 0; i0 < n; i0 += SUB) {
            int m = std::min(SUB, n - i0);
            update(s);
            for (int i = 0; i < m; ++i) d[i0 + i] = tick(d[i0 + i], s);
        }
    }

    // Mise à jour des coefficients (tous les 64 échantillons)
    void update(const Settings& s) {
        const float* p = s.p; const float aiA = p[P::ai];
        // --- IA : normalisation de niveau + correction spectrale adaptative
        float fullDb = linToDb(std::sqrt(fullPow));
        if (aiA < 0.001f) { aiGainDb = 0; aiTilt = 0; aiMud = 0; }
        else if (fullDb > -50.f) {
            float tg = clampf((-20.f - fullDb) * 0.8f, -12.f, 12.f) * aiA;
            aiGainDb += clampf(tg - aiGainDb, -0.01f, 0.01f);
            float hr = std::sqrt(highPow / (fullPow + 1e-12f)), mr = std::sqrt(lowPow / (fullPow + 1e-12f));
            aiTilt += (clampf((0.12f - hr) * 30.f, -3.f, 3.f) * aiA - aiTilt) * 0.01f;
            aiMud  += (clampf((mr - 0.5f) * 12.f, 0.f, 4.f) * aiA - aiMud) * 0.01f;
        }
        aiGainTarget = dbToLin(aiGainDb);
        float vf = 1.f; if (f0Avg > 0) vf = f0Avg < 165.f ? 0.88f : (f0Avg > 240.f ? 1.12f : 1.f);
        vf = 1.f + (vf - 1.f) * aiA;
        // --- gate
        gateThrLin = dbToLin(p[P::gateThr]); gRel = coefMs(p[P::gateRel], fs);
        // --- anti-boom
        hpf1.highPass(fs, p[P::boomHpf], 0.7071f); hpf2.highPass(fs, p[P::boomHpf], 0.7071f);
        float ratio = boomLo / (boomFull + 1e-6f), ex = clampf((ratio - 0.45f) / 0.4f, 0.f, 1.f);
        boomDb += (-p[P::boomAmt] * 10.f * ex - boomDb) * 0.2f; boomCut.peak(fs, 180.f, 1.2f, boomDb);
        // --- de-esser
        float f = clampf(p[P::dsFreq] * vf, 2500.f, 12000.f);
        dsDet.bandPass(fs, f, 1.6f);
        float tg = clampf((linToDb(dsEnv) - p[P::dsThr]) * 0.9f, 0.f, p[P::dsAmt]);
        dsRed += (tg - dsRed) * 0.4f; dsCut.peak(fs, f, 1.6f, -dsRed);
        // --- EQ
        eqL.lowShelf(fs, 120.f, p[P::eqLow]); eqB.peak(fs, 350.f, 1.f, p[P::eqBody] - aiMud);
        eqP.peak(fs, 3500.f, 0.9f, p[P::eqPres]); eqA.highShelf(fs, 10000.f, p[P::eqAir] + aiTilt);
        // --- adoucisseur
        float sf = p[P::soft]; softP.peak(fs, 3800.f, 0.8f, -6.f * sf); softS.highShelf(fs, 9000.f, -4.f * sf);
        // --- compresseur
        cAtkC = coefMs(p[P::cAtk], fs); cRelC = coefMs(p[P::cRel], fs);
        compThr = p[P::cThr]; compRatio = std::max(1.f, p[P::cRatio]);
        compMakeup = std::min(18.f, -compThr * (1.f - 1.f / compRatio) * 0.5f);
        ptC = coefMs(p[P::ptSpeed], fs);
    }

    void onPitch(float f0, const Settings& s) {
        if (f0 > 60.f && f0 < 900.f) {
            voiced = true; float m = 69.f + 12.f * std::log2(f0 / 440.f);
            midiRaw = std::fabs(m - midiRaw) > 1.5f ? m : midiRaw + (m - midiRaw) * 0.5f;
            f0Avg = f0Avg > 0 ? f0Avg + (f0 - f0Avg) * 0.02f : f0;
            midiSnap = snapNote(midiRaw, s.key, s.scale);
            int a = (int) std::lround(s.p[P::h1]), b = (int) std::lround(s.p[P::h2]);
            hm1 = a ? stepNote(midiSnap, a, s.key, s.scale) : midiSnap;
            hm2 = b ? stepNote(midiSnap, b, s.key, s.scale) : midiSnap;
        } else voiced = false;
    }

    inline float tick(float in, const Settings& s) {
        // Never allow NaN/Inf from an upstream host or corrupted state to poison filter memory.
        if (!std::isfinite(in)) in = 0.0f;
        in = clampf(in, -32.0f, 32.0f);
        const float* p = s.p;
        // analyse IA sur l'entrée brute
        fullPow += (in * in - fullPow) * anC;
        { float l = anLow.process(in); lowPow += (l * l - lowPow) * anC; float h = anHigh.process(in); highPow += (h * h - highPow) * anC; }
        aiGainLin += (aiGainTarget - aiGainLin) * 0.002f;
        float x = in * aiGainLin;

        // --- gate (avec maintien)
        float ax = std::fabs(x); gateEnv = ax > gateEnv ? ax : gateEnv + (ax - gateEnv) * gEnvR;
        float tgt;
        if (gateEnv > gateThrLin) { gateHold = (int) (0.06 * fs); tgt = 1.f; }
        else if (gateHold > 0) { --gateHold; tgt = 1.f; } else tgt = 0.02f;
        gateGain += (tgt - gateGain) * (tgt > gateGain ? gAtk : gRel);
        x *= gateGain;

        // --- anti-boom
        x = hpf2.process(hpf1.process(x));
        { float lo = boomDet.process(x); boomLo += (std::fabs(lo) - boomLo) * envC; boomFull += (std::fabs(x) - boomFull) * envC; }
        x = boomCut.process(x);

        // --- détection de hauteur + correction
        if ((pitchEnabled || harmonyEnabled) && pd.push(x)) onPitch(pd.f0, s);
        float tgtShift = voiced ? (midiSnap - midiRaw) * p[P::ptAmt] : 0.f;
        shiftSemi += (tgtShift - shiftSemi) * ptC;
        float dl = dry.process(x), y = dl;
        if (pitchEnabled) {
            float w = shMain.process(x, std::exp2(shiftSemi * (1.f / 12.f)));
            float b = std::min(1.f, std::fabs(shiftSemi) * 20.f);   // évite le peigne quand aucune correction
            y = dl + (w - dl) * b;
        }
        // --- harmonies (calées sur la gamme)
        float hv = 0.f; const float hm = p[P::hMix];
        const int a = (int) std::lround(p[P::h1]), b2 = (int) std::lround(p[P::h2]);
        voicedGain += ((voiced ? 1.f : 0.f) - voicedGain) * vgC;
        if (harmonyEnabled && hm > 0.001f && (a || b2)) {
            if (a)  { hs1 += ((voiced ? hm1 - midiRaw : hs1) - hs1) * hC; hv += sh1.process(x, std::exp2(hs1 * (1.f / 12.f))); }
            if (b2) { hs2 += ((voiced ? hm2 - midiRaw : hs2) - hs2) * hC; hv += sh2.process(x, std::exp2(hs2 * (1.f / 12.f))); }
            hv *= hm * voicedGain * 0.7f;
        }
        x = y;

        // --- de-esser
        { float band = std::fabs(dsDet.process(x)); dsEnv += (band - dsEnv) * (band > dsEnv ? dsA : dsR); }
        x = dsCut.process(x);

        // --- EQ
        x = eqA.process(eqP.process(eqB.process(eqL.process(x))));

        // --- compresseur (genou doux 6 dB, makeup auto)
        { float a2 = std::fabs(x); compEnv += (a2 - compEnv) * (a2 > compEnv ? cAtkC : cRelC);
          float red = 0.f;
          if (compEnv > 1e-5f) {
              float over = linToDb(compEnv) - compThr, k = 1.f - 1.f / compRatio;
              if (over >= 3.f) red = over * k; else if (over > -3.f) { float t = over + 3.f; red = k * t * t / 12.f; }
          }
          x *= std::exp((compMakeup - red) * 0.1151293f); }

        // --- saturation parallèle
        { float mix = p[P::satMix], dr = p[P::satDrive];
          if (mix > 0.001f && dr > 0.001f) {
              float g = 1.f + dr * 7.f, wet = (std::tanh(g * x + 0.1f) - 0.0997f) / (1.f + (g - 1.f) * 0.45f);
              x = x * (1.f - mix) + wet * mix;
          } }

        // --- adoucisseur de voix agressives
        if (p[P::soft] > 0.001f) x = softS.process(softP.process(x));

        // --- sécurité finale : suppression du continu et limiteur de crête.
        // Le limiteur réduit le gain immédiatement et le relâche progressivement.
        float out = x + hv;
        float dc = out - dcX1 + 0.995f * dcY1; dcX1 = out; dcY1 = dc;
        return std::isfinite(dc) ? dc : 0.f;
    }
};

// ------------------------------------------------------------------ Delay stéréo (feedback assombri)
struct Delay {
    std::vector<float> b[2]; int mask = 0, w = 0; double fs = 48000.0; float lp[2] = {0, 0}, cur = 12000.f;
    void prepare(double sr) {
        fs = sr; int n = 1; while (n < (int) (sr * 2.2)) n <<= 1;
        for (auto& v : b) { v.assign((size_t) n, 0.f); }
        mask = n - 1; w = 0; lp[0] = lp[1] = 0; cur = 0.25f * (float) sr;
    }
    void reset() { for (auto& v : b) std::fill(v.begin(), v.end(), 0.0f); w = 0; lp[0] = lp[1] = 0.0f; }
    void process(float** ch, int nch, int n, float ms, float fb, float mix) {
        float target = ms * 0.001f * (float) fs;
        for (int i = 0; i < n; ++i) {
            cur += (target - cur) * 0.0005f;
            for (int c = 0; c < nch; ++c) {
                float rp = (float) w - cur * (c == 1 ? 1.07f : 1.f);
                int i0 = (int) std::floor(rp); float fr = rp - (float) i0;
                float y = b[c][(size_t) (i0 & mask)] * (1.f - fr) + b[c][(size_t) ((i0 + 1) & mask)] * fr;
                lp[c] += (y - lp[c]) * 0.35f;
                float in = ch[c][i]; b[c][(size_t) w] = in + lp[c] * fb; ch[c][i] = in + y * mix;
            }
            w = (w + 1) & mask;
        }
    }
};


// Linked stereo look-ahead peak limiter. The detector sees the undelayed signal,
// while audio is delayed by a short fixed window. No allocation occurs in process().
struct LookaheadLimiter {
    std::vector<float> delay[2];
    // Monotonic deque containing the maximum linked-stereo peak in the
    // look-ahead window. Preallocated in prepare(); no audio-thread allocation.
    std::vector<float> peakValues;
    std::vector<unsigned long long> peakIndices;
    int dequeMask = 0, dequeHead = 0, dequeTail = 0;
    unsigned long long sampleIndex = 0;
    int write = 0, mask = 0, lookahead = 1, hold = 0;
    bool wasLimiting = false;
    float gain = 1.0f;
    double sampleRate = 48000.0;

    void prepare(double sr, float lookaheadMs = 5.0f) {
        sampleRate = std::max(8000.0, sr);
        lookahead = std::max(1, (int) std::lround(sampleRate * lookaheadMs * 0.001));
        int size = 1; while (size < lookahead + 2) size <<= 1;
        for (auto& channel : delay) channel.assign((size_t) size, 0.0f);
        mask = size - 1;
        int dqSize = 1; while (dqSize < lookahead + 4) dqSize <<= 1;
        peakValues.assign((size_t) dqSize, 0.0f);
        peakIndices.assign((size_t) dqSize, 0ULL);
        dequeMask = dqSize - 1;
        reset();
    }
    int latency() const { return lookahead; }
    void reset() {
        for (auto& channel : delay) std::fill(channel.begin(), channel.end(), 0.0f);
        std::fill(peakValues.begin(), peakValues.end(), 0.0f);
        std::fill(peakIndices.begin(), peakIndices.end(), 0ULL);
        write = dequeHead = dequeTail = 0;
        sampleIndex = 0; gain = 1.0f; hold = 0; wasLimiting = false;
    }
    void process(float** channels, int count, int samples, float outputGain, float ceilingDb, float releaseMs) {
        const float ceiling = dbToLin(clampf(ceilingDb, -12.0f, -0.05f));
        const float rel = coefMs(clampf(releaseMs, 10.0f, 500.0f), sampleRate);
        for (int i = 0; i < samples; ++i, ++sampleIndex) {
            float peak = 0.0f;
            float input[2] = {0.0f, 0.0f};
            for (int c = 0; c < count; ++c) {
                float v = channels[c][i];
                if (!std::isfinite(v)) v = 0.0f;
                input[c] = clampf(v * outputGain, -64.0f, 64.0f);
                peak = std::max(peak, std::fabs(input[c]));
            }

            // Push current linked peak; discard smaller values from the tail.
            while (dequeHead != dequeTail) {
                const int last = (dequeTail - 1) & dequeMask;
                if (peakValues[(size_t) last] > peak) break;
                dequeTail = last;
            }
            peakValues[(size_t) dequeTail] = peak;
            peakIndices[(size_t) dequeTail] = sampleIndex;
            dequeTail = (dequeTail + 1) & dequeMask;

            // Keep [current-lookahead, current], i.e. every sample that can
            // emerge at the output now or during the upcoming look-ahead span.
            const unsigned long long oldest = sampleIndex > (unsigned long long) lookahead
                ? sampleIndex - (unsigned long long) lookahead : 0ULL;
            while (dequeHead != dequeTail && peakIndices[(size_t) dequeHead] < oldest)
                dequeHead = (dequeHead + 1) & dequeMask;

            const float windowPeak = dequeHead != dequeTail ? peakValues[(size_t) dequeHead] : 0.0f;
            const float wanted = windowPeak > ceiling ? ceiling / (windowPeak + 1.0e-12f) : 1.0f;
            if (wanted < gain) {
                gain = wanted; hold = lookahead; wasLimiting = true;
            } else {
                if (hold > 0) --hold;
                else gain += (1.0f - gain) * rel;
                if (gain > 0.9999f) { gain = 1.0f; wasLimiting = false; }
            }

            for (int c = 0; c < count; ++c) {
                const float delayed = delay[c][(size_t) ((write - lookahead) & mask)];
                delay[c][(size_t) write] = input[c];
                const float out = delayed * gain;
                channels[c][i] = std::isfinite(out) ? out : 0.0f;
            }
            write = (write + 1) & mask;
        }
    }
};

} // namespace bvm

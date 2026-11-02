#include "../Source/Dsp.h"
#include <iostream>
#include <vector>
#include <cmath>

static bool finite(const std::vector<float>& v) {
    for (float x : v) if (!std::isfinite(x)) return false;
    return true;
}

int main() {
    using namespace bvm;
    auto settings = defaultSettings();
    Channel voice; voice.prepare(48000.0);
    std::vector<float> signal(48000, 0.0f);
    for (size_t i = 0; i < signal.size(); ++i)
        signal[i] = 0.18f * std::sin(2.0 * 3.141592653589793 * 220.0 * double(i) / 48000.0);
    voice.processBlock(signal.data(), (int) signal.size(), settings);
    if (!finite(signal)) return 1;

    // The expensive YIN stage must remain dormant when pitch and harmonies are off.
    Channel economical; economical.prepare(48000.0);
    std::vector<float> drySignal(48000, 0.1f);
    economical.processBlock(drySignal.data(), (int) drySignal.size(), settings);
    if (economical.pd.analysisCount != 0) return 8;

    // Pitch analysis is enabled only when requested, and must run on valid audio.
    Settings tuned = settings; tuned.p[P::pitchOn] = 1.0f;
    Channel tunedVoice; tunedVoice.prepare(48000.0);
    tunedVoice.processBlock(signal.data(), (int) signal.size(), tuned);
    if (tunedVoice.pd.analysisCount == 0) return 9;

    // Corrupted upstream samples must not contaminate DSP state.
    float bad[] = { NAN, INFINITY, -INFINITY, 0.0f, 0.25f };
    voice.processBlock(bad, 5, settings);
    for (float x : bad) if (!std::isfinite(x)) return 2;

    // Linked look-ahead limiter: finite output, correct latency and bounded peaks.
    LookaheadLimiter limiter; limiter.prepare(48000.0, 5.0f);
    const int n = 24000; std::vector<float> left(n, 0.0f), right(n, 0.0f);
    for (int i = 2000; i < 8000; ++i) { left[i] = 2.0f; right[i] = 0.8f; }
    float* stereo[] = { left.data(), right.data() };
    limiter.process(stereo, 2, n, 1.0f, -1.0f, 80.0f);
    if (!finite(left) || !finite(right)) return 3;
    float peak = 0.0f;
    for (float x : left) peak = std::max(peak, std::fabs(x));
    if (peak > dbToLin(-1.0f) * 1.01f) return 4;
    // Verify look-ahead actually catches an upcoming transient before it exits
    // the delay line (output impulse occurs exactly L samples later).
    const int impulseIn = 12000;
    std::vector<float> impulse(16000, 0.0f);
    impulse[impulseIn] = 3.0f;
    LookaheadLimiter transientLimiter; transientLimiter.prepare(48000.0, 5.0f);
    float* mono[] = { impulse.data(), nullptr };
    transientLimiter.process(mono, 1, (int) impulse.size(), 1.0f, -1.0f, 80.0f);
    const int impulseOut = impulseIn + transientLimiter.latency();
    if (impulseOut >= (int) impulse.size()) return 6;
    if (std::fabs(impulse[(size_t) impulseOut]) > dbToLin(-1.0f) * 1.01f) return 7;

    // Multiple common project rates and odd host block sizes remain finite.
    for (double rate : {44100.0, 48000.0, 88200.0, 96000.0, 192000.0}) {
        Channel c; c.prepare(rate);
        std::vector<float> block(257, 0.15f);
        c.processBlock(block.data(), (int) block.size(), settings);
        if (!finite(block)) return 10;
    }

    // Parameter defaults and scale mapping sanity.
    if (P::count < 30 || !inScale(0, 0) || inScale(0, 1)) return 5;
    std::cout << "BvM DSP tests passed; YIN skipped when unused; parameters=" << P::count
              << "; linked look-ahead limiter latency=" << limiter.latency() << " samples\n";
    return 0;
}

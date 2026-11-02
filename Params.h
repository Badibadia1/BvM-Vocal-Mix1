#pragma once
// BvM - Badi-badia Vocal Mix : table des paramètres + presets (C++ pur, sans JUCE)
#include <cmath>
#include <algorithm>
#include <vector>
#include <utility>

namespace bvm {

// X(id, libellé, min, max, défaut, skew, pas)
#define BVM_PARAMS(X)                                                   \
 X(ai,       "IA Voix",        0.f,    1.f,    0.7f,  1.f,  0.f)        \
 X(gateThr,  "Seuil dB",      -80.f,  -20.f,  -55.f,  1.f,  0.1f)       \
 X(gateRel,  "Relâche ms",     20.f,  500.f,  120.f,  0.5f, 1.f)        \
 X(boomHpf,  "Coupe Hz",       20.f,  200.f,   80.f,  0.5f, 1.f)        \
 X(boomAmt,  "Anti-boom",       0.f,    1.f,    0.5f, 1.f,  0.f)        \
 X(dsFreq,   "Freq Hz",      3000.f,10000.f, 6500.f,  0.6f, 10.f)       \
 X(dsThr,    "Seuil dB",      -50.f,  -10.f,  -30.f,  1.f,  0.1f)       \
 X(dsAmt,    "Réduc. dB",       0.f,   12.f,    6.f,  1.f,  0.1f)       \
 X(cThr,     "Seuil dB",      -40.f,    0.f,  -18.f,  1.f,  0.1f)       \
 X(cRatio,   "Ratio",           1.f,   10.f,    3.f,  0.6f, 0.1f)       \
 X(cAtk,     "Attaque ms",      0.1f,  80.f,    8.f,  0.4f, 0.1f)       \
 X(cRel,     "Relâche ms",     20.f,  800.f,  120.f,  0.5f, 1.f)        \
 X(eqLow,    "Grave",         -12.f,   12.f,    0.f,  1.f,  0.1f)       \
 X(eqBody,   "Corps",         -12.f,   12.f,    0.f,  1.f,  0.1f)       \
 X(eqPres,   "Présence",      -12.f,   12.f,    2.f,  1.f,  0.1f)       \
 X(eqAir,    "Air",           -12.f,   12.f,    2.f,  1.f,  0.1f)       \
 X(satDrive, "Drive",           0.f,    1.f,    0.15f,1.f,  0.f)        \
 X(satMix,   "Mix",             0.f,    1.f,    0.3f, 1.f,  0.f)        \
 X(soft,     "Douceur",         0.f,    1.f,    0.f,  1.f,  0.f)        \
 X(pitchOn,  "Actif",           0.f,    1.f,    0.f,  1.f,  1.f)        \
 X(ptSpeed,  "Vitesse ms",      1.f,  200.f,   25.f,  0.5f, 1.f)        \
 X(ptAmt,    "Force",           0.f,    1.f,    1.f,  1.f,  0.f)        \
 X(h1,       "Voix 1",         -7.f,    7.f,    0.f,  1.f,  1.f)        \
 X(h2,       "Voix 2",         -7.f,    7.f,    0.f,  1.f,  1.f)        \
 X(hMix,     "Niveau",          0.f,    1.f,    0.4f, 1.f,  0.f)        \
 X(dTime,    "Temps ms",       20.f, 1000.f,  250.f,  0.5f, 1.f)        \
 X(dFb,      "Répét.",          0.f,    0.9f,   0.3f, 1.f,  0.f)        \
 X(dMix,     "Mix",             0.f,    1.f,    0.f,  1.f,  0.f)        \
 X(rSize,    "Taille",          0.f,    1.f,    0.4f, 1.f,  0.f)        \
 X(rDamp,    "Amorti",          0.f,    1.f,    0.5f, 1.f,  0.f)        \
 X(rMix,     "Mix",             0.f,    1.f,    0.15f,1.f,  0.f)        \
 X(outGain,  "Sortie dB",     -24.f,   12.f,    0.f,  1.f,  0.1f)        \
 X(limCeil,  "Plafond dB",     -6.f,   -0.1f,   -1.f,  1.f,  0.1f)       \
 X(limRel,   "Relâche ms",     20.f,  300.f,   80.f,  0.5f, 1.f)

struct P {
    enum {
#define X(id, ...) id,
        BVM_PARAMS(X)
#undef X
        count
    };
};

struct Settings {
    float p[P::count];
    int key = 0;    // 0 = Do ... 11 = Si
    int scale = 0;  // 0 majeur, 1 mineur, 2 chromatique, 3 pentatonique mineure
};

inline Settings defaultSettings() {
    Settings s;
    int i = 0;
#define X(id, label, mn, mx, df, sk, st) s.p[i++] = df;
    BVM_PARAMS(X)
#undef X
    return s;
}

// ---------------------------------------------------------------- Presets
using KV = std::pair<const char*, float>;
struct Preset { const char* name; std::vector<KV> v; };

// Seules les valeurs différentes des défauts sont listées. "key"/"scale" = index.
inline const std::vector<Preset>& presets() {
    static const std::vector<Preset> list = {
        {"Rap moderne - Punch", {{"ai",.8f},{"gateThr",-50},{"boomHpf",90},{"boomAmt",.6f},{"dsAmt",7},{"cThr",-22},{"cRatio",4},{"cAtk",5},{"cRel",90},{"eqLow",1},{"eqBody",-2},{"eqPres",3},{"eqAir",2.5f},{"satDrive",.3f},{"satMix",.35f},{"dTime",180},{"dMix",.12f},{"rMix",.08f}}},
        {"Trap - Auto-tune dur", {{"pitchOn",1},{"ptSpeed",1},{"ptAmt",1},{"key",0},{"scale",1},{"cThr",-20},{"cRatio",4},{"eqPres",3},{"eqAir",4},{"satDrive",.25f},{"h1",2},{"h2",4},{"hMix",.25f},{"dTime",300},{"dFb",.35f},{"dMix",.2f},{"rSize",.5f},{"rMix",.2f}}},
        {"Pop - Brillant", {{"pitchOn",1},{"ptSpeed",40},{"ptAmt",.8f},{"scale",0},{"cRatio",3},{"eqAir",4},{"eqPres",2.5f},{"dsAmt",8},{"satMix",.2f},{"dMix",.1f},{"rMix",.22f}}},
        {"R&B - Velours", {{"pitchOn",1},{"ptSpeed",80},{"ptAmt",.6f},{"scale",1},{"soft",.5f},{"cThr",-20},{"cRatio",2.5f},{"eqLow",2},{"eqBody",-1},{"eqAir",1.5f},{"satDrive",.2f},{"h1",2},{"h2",-3},{"hMix",.35f},{"rSize",.6f},{"rMix",.3f}}},
        {"Afrobeats - Chaleur", {{"pitchOn",1},{"ptSpeed",20},{"ptAmt",.9f},{"scale",3},{"cRatio",3.5f},{"eqLow",1},{"eqPres",2.5f},{"eqAir",3},{"satMix",.3f},{"h1",2},{"hMix",.25f},{"dTime",220},{"dMix",.15f},{"rMix",.15f}}},
        {"Gospel - Chœur", {{"h1",2},{"h2",4},{"hMix",.6f},{"cRatio",2.5f},{"soft",.2f},{"eqAir",2},{"rSize",.7f},{"rMix",.35f}}},
        {"Podcast - Parole", {{"gateThr",-45},{"boomHpf",100},{"boomAmt",.7f},{"cThr",-20},{"cRatio",3},{"eqLow",-1},{"eqPres",3},{"eqAir",1},{"dsAmt",8},{"satMix",.1f},{"rMix",0}}},
        {"Voix aiguë - Adoucie", {{"soft",.6f},{"dsFreq",8000},{"dsAmt",9},{"eqPres",0},{"eqAir",1.5f},{"cRatio",3},{"rMix",.2f}}},
        {"Voix grave - Autorité", {{"boomHpf",70},{"boomAmt",.7f},{"eqBody",-3},{"eqPres",3.5f},{"eqAir",3},{"cRatio",3.5f},{"dsFreq",5500}}},
        {"Voix agressive - Douce", {{"soft",1},{"dsThr",-34},{"dsAmt",10},{"cRatio",3},{"eqPres",-1.5f},{"eqAir",.5f},{"satMix",.1f}}},
        {"Nettoyage home studio", {{"ai",1},{"gateThr",-42},{"boomHpf",110},{"boomAmt",.8f},{"dsAmt",7},{"cRatio",3}}},
        {"Naturel transparent", {{"ai",.5f},{"cThr",-16},{"cRatio",2},{"eqPres",1},{"eqAir",1.5f},{"satMix",0},{"rMix",.1f}}},
    };
    return list;
}

} // namespace bvm

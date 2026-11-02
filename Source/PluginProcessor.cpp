#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace bvm;

juce::AudioProcessorValueTreeState::ParameterLayout BvMProcessor::makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto dec = [](float mn, float mx, float st) { return st >= 1.f ? 0 : ((mx - mn) > 100.f ? 0 : ((mx - mn) >= 5.f ? 1 : 2)); };
    auto add = [&l, &dec](const char* id, const char* label, float mn, float mx, float df, float sk, float st)
    {
        if (juce::String(id) == "pitchOn")     // interrupteur Auto-Tune : vrai booléen pour la DAW
        {
            l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ id, 1 }, juce::String::fromUTF8(label), df > 0.5f));
            return;
        }
        const int d = dec(mn, mx, st);
        l.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ id, 1 }, juce::String::fromUTF8(label),
            juce::NormalisableRange<float>(mn, mx, st, sk), df,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction(
                [d](float v, int) { return juce::String(v, d); })));
    };
#define X(id, label, mn, mx, df, sk, st) add(#id, label, mn, mx, df, sk, st);
    BVM_PARAMS(X)
#undef X
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "key", 1 }, juce::String::fromUTF8("Tonalité"),
          juce::StringArray{ "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" }, 0));
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "scale", 1 }, "Gamme",
          juce::StringArray{ "Majeur", "Mineur", "Chromatique", "Pentatonique mineure" }, 0));
    return l;
}

BvMProcessor::BvMProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "BvM", makeLayout())
{
    int i = 0;
#define X(id, ...) raw[i++] = apvts.getRawParameterValue(#id);
    BVM_PARAMS(X)
#undef X
    keyP = apvts.getRawParameterValue("key");
    scaleP = apvts.getRawParameterValue("scale");
    startTimer(15000);   // revérifie la licence toutes les 15 s (hors thread audio)
}

bool BvMProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void BvMProcessor::prepareToPlay(double sr, int)
{
    for (auto& c : ch) c.prepare(sr);
    delay.prepare(sr);
    reverb.setSampleRate(sr); reverb.reset(); reverbWasActive = false;
    limiter.prepare(sr, 5.0f);
    setLatencySamples(ch[0].latency() + limiter.latency());
}

void BvMProcessor::processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buf.getNumSamples(), nch = std::min(buf.getNumChannels(), 2);
    for (int c = nch; c < buf.getNumChannels(); ++c) buf.clear(c, 0, n);

    if (isLocked()) { buf.clear(); return; }     // essai terminé : silence jusqu'à activation

    Settings s;
    for (int i = 0; i < P::count; ++i) s.p[i] = raw[i]->load();
    s.key = (int) keyP->load();
    s.scale = std::min(std::max((int) scaleP->load(), 0), 3);

    for (int c = 0; c < nch; ++c) ch[c].processBlock(buf.getWritePointer(c), n, s);

    float* ptrs[2] = { buf.getWritePointer(0), nch > 1 ? buf.getWritePointer(1) : nullptr };
    // Le delay reste alimenté pour conserver une mémoire temporelle cohérente
    // si son mix est réactivé en cours de session.
    delay.process(ptrs, nch, n, s.p[P::dTime], s.p[P::dFb], s.p[P::dMix]);

    const bool reverbActive = s.p[P::rMix] > 0.0001f;
    if (reverbActive) {
        // juce::Reverb multiplie le dry par 2 en interne : dryLevel = 0.5 => gain unitaire.
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.3f + 0.65f * s.p[P::rSize]; rp.damping = s.p[P::rDamp];
        rp.wetLevel = s.p[P::rMix] * 0.5f; rp.dryLevel = 0.5f; rp.width = 1.f; rp.freezeMode = 0.f;
        reverb.setParameters(rp);
        if (nch == 2) reverb.processStereo(ptrs[0], ptrs[1], n); else reverb.processMono(ptrs[0], n);
    } else if (reverbWasActive) {
        reverb.reset();
    }
    reverbWasActive = reverbActive;

    float* limiterChannels[2] = { buf.getWritePointer(0), nch > 1 ? buf.getWritePointer(1) : nullptr };
    limiter.process(limiterChannels, nch, n, dbToLin(s.p[P::outGain]), s.p[P::limCeil], s.p[P::limRel]);
}

void BvMProcessor::applyPreset(int i)
{
    const auto& list = presets();
    if (i < 0 || i >= (int) list.size()) return;
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(prm))
            r->setValueNotifyingHost(r->getDefaultValue());
    for (auto& kv : list[(size_t) i].v)
        if (auto* r = apvts.getParameter(kv.first))
            r->setValueNotifyingHost(r->convertTo0to1(kv.second));
    apvts.state.setProperty("preset", i, nullptr);   // mémorisé dans le projet de la DAW
}

void BvMProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void BvMProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* BvMProcessor::createEditor() { return new BvMEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BvMProcessor(); }

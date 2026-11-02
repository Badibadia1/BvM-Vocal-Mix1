#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "Dsp.h"
#include "License.h"

class BvMProcessor : public juce::AudioProcessor, private juce::Timer {
public:
    BvMProcessor();
    ~BvMProcessor() override { stopTimer(); }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BvM"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    // Les présets sont gérés par l'interface (menu du plugin) et NON exposés comme "programmes" de la DAW :
    // sinon certaines DAW rappellent le programme 0 à l'ouverture d'un projet et écrasent les réglages sauvegardés.
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "BvM"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    void applyPreset(int index);
    int getPresetIndex() const { return (int) apvts.state.getProperty("preset", -1); }
    bool isLocked() const { return bvm::License::now() >= license.expiry.load(); }

    juce::AudioProcessorValueTreeState apvts;
    bvm::License license;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    void timerCallback() override { license.refresh(); }

    bvm::Channel ch[2];
    bvm::Delay delay;
    juce::Reverb reverb;
    std::atomic<float>* raw[bvm::P::count] {};
    std::atomic<float>* keyP = nullptr;
    std::atomic<float>* scaleP = nullptr;
    bvm::LookaheadLimiter limiter;
    bool reverbWasActive = false;
};

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "engine/SynthEngine.h"

class BombSynthAudioProcessor : public juce::AudioProcessor {
public:
    BombSynthAudioProcessor();
    ~BombSynthAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout&) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool  acceptsMidi()  const override { return true; }
    bool  producesMidi() const override { return false; }
    bool  isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int  getNumPrograms()                         override { return 1; }
    int  getCurrentProgram()                      override { return 0; }
    void setCurrentProgram(int)                   override {}
    const juce::String getProgramName(int)        override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int)   override;

    juce::AudioProcessorValueTreeState& parameters() { return params_; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    juce::AudioProcessorValueTreeState params_ { *this, nullptr, "BombSynth",
                                                  createParameters() };
    SynthEngine engine_;

    // ── Effects ───────────────────────────────────────────────────────────────
    juce::dsp::Reverb           reverb_;
    juce::dsp::Chorus<float>    chorus_;

    // Stereo delay (ring buffers, max 2s)
    static constexpr int kMaxDelaySamples = 192000;
    std::array<std::vector<float>, 2> delayBuf_;
    int   delayWrite_  = 0;
    float delaySmoothL_ = 0.f, delaySmoothR_ = 0.f;  // simple smoothed feedback accum

    double sampleRate_ = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BombSynthAudioProcessor)
};

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "engine/SynthEngine.h"
#include "engine/LFOEngine.h"
#include "engine/Sequencer.h"

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

    // User wavetable persistence
    void addUserWavetablePath(const juce::String& path);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    juce::AudioProcessorValueTreeState params_ { *this, nullptr, "BombSynth",
                                                  createParameters() };
    SynthEngine engine_;
    Sequencer   sequencer_;

    // LFOs
    LFOEngine lfo1_, lfo2_;

    // Accumulated mod values (written in processBlock, read by engine)
    float modCutoff_      = 0.f;
    float modPitch_       = 0.f;
    float modAmp_         = 0.f;
    float modFilterRes_   = 0.f;
    float modFilterDrive_ = 0.f;
    float modWheelValue_  = 0.f;   // updated from MIDI CC1
    std::array<float, 3> modMorph_  { 0.f, 0.f, 0.f };
    std::array<float, 3> modTune_   { 0.f, 0.f, 0.f };
    std::array<float, 3> modFine_   { 0.f, 0.f, 0.f };
    std::array<float, 3> modLevel_  { 0.f, 0.f, 0.f };
    std::array<float, 3> modFM_     { 0.f, 0.f, 0.f };
    std::array<float, 3> modDetune_ { 0.f, 0.f, 0.f };

    // Persisted user wavetable file paths
    juce::StringArray userWavetablePaths_;

public:
    Sequencer& sequencer() { return sequencer_; }
private:

    // ── Effects ───────────────────────────────────────────────────────────────
    juce::dsp::Reverb           reverb_;
    juce::dsp::Chorus<float>    chorus_;

    // Stereo delay (ring buffers, max 2s)
    static constexpr int kMaxDelaySamples = 192000;
    std::array<std::vector<float>, 2> delayBuf_;
    int   delayWrite_  = 0;
    float delaySmoothL_ = 0.f, delaySmoothR_ = 0.f;

    double sampleRate_ = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BombSynthAudioProcessor)
};

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <BinaryData.h>

class BombSynthAudioProcessor;

class BombSynthLookAndFeel : public juce::LookAndFeel_V4 {
public:
    BombSynthLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float pos, float startAngle, float endAngle,
                          juce::Slider&) override;
};

// Reusable labelled knob
class SynthKnob : public juce::Component {
public:
    juce::Slider slider;
    juce::Label  label;
    explicit SynthKnob(const juce::String& name);
    void resized() override;
};

// ─── Main Editor ─────────────────────────────────────────────────────────────
class BombSynthAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    explicit BombSynthAudioProcessorEditor(BombSynthAudioProcessor&);
    ~BombSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    BombSynthAudioProcessor& proc_;
    BombSynthLookAndFeel     laf_;
    juce::Image              logo_;

    // Amp envelope
    SynthKnob attackKnob_  {"Attack"};
    SynthKnob decayKnob_   {"Decay"};
    SynthKnob sustainKnob_ {"Sustain"};
    SynthKnob releaseKnob_ {"Release"};

    // Filter
    SynthKnob cutoffKnob_ {"Cutoff"};
    SynthKnob resKnob_    {"Res"};

    // LFO
    SynthKnob lfo1RateKnob_  {"LFO Rate"};
    SynthKnob lfo1DepthKnob_ {"LFO Depth"};

    // Master
    SynthKnob masterKnob_ {"Volume"};

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAtt> attackAtt_, decayAtt_, sustainAtt_, releaseAtt_;
    std::unique_ptr<SliderAtt> cutoffAtt_, resAtt_;
    std::unique_ptr<SliderAtt> lfo1RateAtt_, lfo1DepthAtt_;
    std::unique_ptr<SliderAtt> masterAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BombSynthAudioProcessorEditor)
};

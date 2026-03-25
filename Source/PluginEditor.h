#pragma once
#include "engine/oscillators/AnalogOscillator.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <BinaryData.h>
#include "ui/WavetableDisplay.h"
#include "ui/EnvelopeDisplay.h"
#include "ui/FilterDisplay.h"
#include "ui/SequencerPanel.h"
#include "ui/ModulationPanel.h"
#include "presets/PresetManager.h"
#include <array>

class BombSynthAudioProcessor;

// ─── Colours ─────────────────────────────────────────────────────────────────
namespace BCol {
    inline const juce::Colour bg      { 0xFF0A0A14 };
    inline const juce::Colour panel   { 0xFF111120 };
    inline const juce::Colour section { 0xFF181828 };
    inline const juce::Colour border  { 0xFF252540 };
    inline const juce::Colour accent  { 0xFFFF3B6F };
    inline const juce::Colour accent2 { 0xFF4FC3F7 };
    inline const juce::Colour green   { 0xFF00E676 };
    inline const juce::Colour amber   { 0xFFFFD740 };
    inline const juce::Colour text    { 0xFFE0E0E0 };
    inline const juce::Colour textDim { 0xFF607080 };
    inline const juce::Colour knobBg  { 0xFF1A1A2E };
}

// ─── Look & Feel ─────────────────────────────────────────────────────────────
class BombLookAndFeel : public juce::LookAndFeel_V4 {
public:
    BombLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float pos, float startAngle, float endAngle,
                          juce::Slider&) override;
    void drawComboBox(juce::Graphics&, int w, int h, bool,
                      int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    void drawLabel(juce::Graphics&, juce::Label&) override;
};

// ─── Labelled knob ────────────────────────────────────────────────────────────
class BKnob : public juce::Component {
public:
    juce::Slider slider;
    juce::Label  label;
    explicit BKnob(const juce::String& name, juce::Colour arcColour = BCol::accent);
    void resized() override;
private:
    juce::Colour arcColour_;
};

// ─── Section panel ───────────────────────────────────────────────────────────
class SectionPanel : public juce::Component {
public:
    explicit SectionPanel(const juce::String& title, juce::Colour headerColour = BCol::accent);
    void paint(juce::Graphics&) override;
    juce::Rectangle<int> getContentArea() const;
private:
    juce::String title_;
    juce::Colour headerColour_;
};

// ─── Oscillator strip (one row per OSC) ──────────────────────────────────────
class OscStrip : public juce::Component, public juce::Slider::Listener {
public:
    WavetableDisplay display;

    // WT knobs
    BKnob morphKnob  { "MORPH",   BCol::accent2 };
    // Octave spinner (drawn in paint, click handled in mouseDown)
    int   octaveValue_ = 0;   // -3..+3, kept in sync with APVTS
    juce::Rectangle<int> octDecR_, octIncR_, octDisplayR_;

    BKnob tuneKnob   { "TUNE",   BCol::accent2 };
    BKnob fineKnob   { "FINE",    BCol::accent2 };
    BKnob levelKnob  { "LEVEL",   BCol::accent2 };
    BKnob fmKnob     { "FM",      BCol::amber   };
    BKnob unisonKnob { "UNISON",  BCol::textDim };
    BKnob detuneKnob { "DETUNE",  BCol::textDim };

    // Granular knobs (visible in GR mode)
    BKnob densityKnob { "DENSITY", BCol::green };
    BKnob sizeKnob    { "SIZE",    BCol::green };
    BKnob sprayKnob   { "SPRAY",   BCol::green };
    BKnob pitchScKnob { "PITCH",   BCol::green };

    // Warp controls (always visible)
    juce::TextButton warpModeBtn { "WARP" };
    BKnob warpAmtKnob { "AMT", BCol::accent };

    // Engine mode toggle
    juce::TextButton modeBtn { "WT" };

    std::function<void(int engineIdx)> onModeChanged;
    std::function<void(int mode)>    onWarpModeChanged;

    explicit OscStrip(int index);
    ~OscStrip() override;
    void resized() override;
    void paint(juce::Graphics&) override;

    void setEngineIdx(int idx);
    void setGranMode(bool gran) { setEngineIdx(gran ? OscEngine::GR : OscEngine::WT); }  // compat
    void setWarpModeDisplay(int mode);

    // juce::Slider::Listener — updates display morph when knob moves
    void sliderValueChanged(juce::Slider* s) override;
    void mouseDown(const juce::MouseEvent& e) override;

    std::function<void(int)> onOctaveChanged;  // called with new octave value

    int  engineIdx() const { return engineIdx_; }
    bool granMode()  const { return engineIdx_ == OscEngine::GR; }
    int  warpModeVal() const { return warpMode_; }

private:
    int  index_;
    int engineIdx_ = OscEngine::WT;
    int  warpMode_ = 0;
    static constexpr int kLabelW   = 44;
    static constexpr int kDisplayW = 155;
    static constexpr int kWarpBtnW = 34;
    static constexpr int kWarpKnobW= 52;
};

// ─── Main Editor ─────────────────────────────────────────────────────────────
class BombSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer {
public:
    explicit BombSynthAudioProcessorEditor(BombSynthAudioProcessor&);
    ~BombSynthAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    BombSynthAudioProcessor& proc_;
    BombLookAndFeel laf_;
    juce::Image logo_;

    // ── OSCs ─────────────────────────────────────────────────────────────────
    SectionPanel oscSection_ { "OSCILLATORS", BCol::accent2 };
    std::array<OscStrip, 3> oscs_ { OscStrip{1}, OscStrip{2}, OscStrip{3} };

    // ── Filter ───────────────────────────────────────────────────────────────
    SectionPanel filterSection_  { "FILTER",  BCol::amber };
    juce::Label    filterTypeLabel_;
    juce::ComboBox filterTypeBox_;
    FilterDisplay  filterDisplay_;
    BKnob cutoffKnob_ { "CUTOFF",    BCol::amber };
    BKnob resKnob_    { "RESONANCE", BCol::amber };
    BKnob driveKnob_  { "DRIVE",     BCol::accent };
    BKnob envAmtKnob_ { "ENV AMT",   BCol::green  };

    // ── Amp Envelope ─────────────────────────────────────────────────────────
    SectionPanel ampEnvSection_ { "AMP ENVELOPE",    BCol::green };
    BKnob ampAttKnob_ { "ATTACK",  BCol::green };
    BKnob ampDecKnob_ { "DECAY",   BCol::green };
    BKnob ampSusKnob_ { "SUSTAIN", BCol::green };
    BKnob ampRelKnob_ { "RELEASE", BCol::green };
    BKnob ampCrvKnob_ { "CURVE",   BCol::textDim };

    // ── Filter Envelope ───────────────────────────────────────────────────────
    SectionPanel filterEnvSection_ { "FILTER ENVELOPE", BCol::amber };
    BKnob fEnvAttKnob_ { "ATTACK",  BCol::amber };
    BKnob fEnvDecKnob_ { "DECAY",   BCol::amber };
    BKnob fEnvSusKnob_ { "SUSTAIN", BCol::amber };
    BKnob fEnvRelKnob_ { "RELEASE", BCol::amber };

    // ── LFO 1 ────────────────────────────────────────────────────────────────
    SectionPanel lfo1Section_ { "LFO 1", BCol::accent };
    juce::Label    lfo1ShapeLabel_;
    juce::ComboBox lfo1ShapeBox_;
    BKnob lfo1RateKnob_  { "RATE",  BCol::accent };
    BKnob lfo1DepthKnob_ { "DEPTH", BCol::accent };
    BKnob lfo1PhaseKnob_ { "PHASE", BCol::textDim };

    // ── LFO 2 ────────────────────────────────────────────────────────────────
    SectionPanel lfo2Section_ { "LFO 2", BCol::accent };
    juce::Label    lfo2ShapeLabel_;
    juce::ComboBox lfo2ShapeBox_;
    BKnob lfo2RateKnob_  { "RATE",  BCol::accent };
    BKnob lfo2DepthKnob_ { "DEPTH", BCol::accent };

    // ── Master ───────────────────────────────────────────────────────────────
    SectionPanel masterSection_ { "MASTER", BCol::textDim };
    BKnob masterVolKnob_ { "VOLUME", BCol::accent };
    BKnob glideKnob_     { "GLIDE",  BCol::accent2 };

    // ── Envelope displays (live ADSR visualisers) ─────────────────────────────
    EnvelopeDisplay ampEnvDisplay_;
    EnvelopeDisplay fEnvDisplay_;

    // ── Tab bar ───────────────────────────────────────────────────────────────
    enum class Tab { Synth, Effects, Modulation, Sequencer };
    Tab activeTab_ = Tab::Synth;
    juce::TextButton synthTabBtn_     { "SYNTH"     };
    juce::TextButton effectsTabBtn_   { "EFFECTS"   };
    juce::TextButton modTabBtn_       { "MOD"       };
    juce::TextButton sequencerTabBtn_ { "SEQUENCER" };

    SequencerPanel  sequencerPanel_;
    std::unique_ptr<ModulationPanel> modPanel_;
    void setTab(Tab t);
    void setSynthVisible(bool v);
    void setEffectsVisible(bool v);
    void setModulationVisible(bool v);
    void setSequencerVisible(bool v);
    void timerCallback() override;

    // ── Effects sections ──────────────────────────────────────────────────────
    static inline const juce::Colour kReverbCol  { 0xFF7986CB };
    static inline const juce::Colour kDelayCol   { 0xFF4DB6AC };
    static inline const juce::Colour kChorusCol  { 0xFFBA68C8 };

    SectionPanel reverbSection_  { "REVERB",  kReverbCol };
    BKnob revRoomKnob_  { "ROOM",     kReverbCol };
    BKnob revDampKnob_  { "DAMP",     kReverbCol };
    BKnob revWidthKnob_ { "WIDTH",    kReverbCol };
    BKnob revWetKnob_   { "WET",      BCol::accent };

    SectionPanel delaySection_   { "DELAY",   kDelayCol  };
    BKnob delTimeKnob_  { "TIME",     kDelayCol  };
    BKnob delFbKnob_    { "FEEDBACK", kDelayCol  };
    BKnob delWetKnob_   { "WET",      BCol::accent };

    SectionPanel chorusSection_  { "CHORUS",  kChorusCol };
    BKnob choRateKnob_  { "RATE",     kChorusCol };
    BKnob choDepthKnob_ { "DEPTH",    kChorusCol };
    BKnob choWetKnob_   { "WET",      BCol::accent };

    // ── Preset browser (header bar) ───────────────────────────────────────────
    juce::ComboBox categoryCombo_;
    juce::ComboBox presetCombo_;
    juce::TextButton prevBtn_ { "<" };
    juce::TextButton nextBtn_ { ">" };
    int currentPresetIdx_ = 0;

    void buildPresetBrowser();
    void populatePresetCombo(const juce::String& category);
    void applyPreset(int idx);
    void syncDisplaysToParams();
    void loadWavetableForOsc(int oscIdx);

    std::unique_ptr<juce::FileChooser> fileChooser_;
    std::vector<juce::File> userWavetableFiles_;

    // Cycling palette for user bank colours
    static constexpr uint32_t kUserColours[] = {
        0xFFE040FB, 0xFF00BCD4, 0xFF8BC34A, 0xFFFF5722,
        0xFF607D8B, 0xFFFF4081, 0xFF69F0AE, 0xFFFF6D00,
        0xFF40C4FF, 0xFFFFD740, 0xFFB0BEC5, 0xFFEA80FC,
    };
    int userColourIdx_ = 0;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using IA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    // OSC knob attachments (no waveBox attachment — display handles bank)
    std::array<std::unique_ptr<SA>, 3> oscMorphAtt_, oscTuneAtt_, oscFineAtt_,
                                        oscLevelAtt_, oscFmAtt_, oscUniAtt_, oscDetuneAtt_;
    // Granular + warp attachments
    std::array<std::unique_ptr<SA>, 3> oscGranDensAtt_, oscGranSizeAtt_,
                                        oscGranSprayAtt_, oscGranPitchAtt_, oscWarpAmtAtt_;
    // Filter
    std::unique_ptr<SA> cutoffAtt_, resAtt_, driveAtt_, envAmtAtt_;
    std::unique_ptr<IA> filterTypeAtt_;
    // Amp env
    std::unique_ptr<SA> ampAttAtt_, ampDecAtt_, ampSusAtt_, ampRelAtt_, ampCrvAtt_;
    // Filter env
    std::unique_ptr<SA> fEnvAttAtt_, fEnvDecAtt_, fEnvSusAtt_, fEnvRelAtt_;
    // LFOs
    std::unique_ptr<SA> lfo1RateAtt_, lfo1DepthAtt_, lfo1PhaseAtt_;
    std::unique_ptr<IA> lfo1ShapeAtt_;
    std::unique_ptr<SA> lfo2RateAtt_, lfo2DepthAtt_;
    std::unique_ptr<IA> lfo2ShapeAtt_;
    // Master
    std::unique_ptr<SA> masterVolAtt_, glideAtt_;
    // Effects
    std::unique_ptr<SA> revRoomAtt_, revDampAtt_, revWidthAtt_, revWetAtt_;
    std::unique_ptr<SA> delTimeAtt_, delFbAtt_,   delWetAtt_;
    std::unique_ptr<SA> choRateAtt_, choDepthAtt_, choWetAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BombSynthAudioProcessorEditor)
};

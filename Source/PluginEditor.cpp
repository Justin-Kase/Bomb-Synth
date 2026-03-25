#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cmath>

// ─── BombLookAndFeel ──────────────────────────────────────────────────────────
BombLookAndFeel::BombLookAndFeel() {
    setColour(juce::Slider::textBoxTextColourId,       BCol::text);
    setColour(juce::Slider::textBoxBackgroundColourId, BCol::knobBg);
    setColour(juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId,               BCol::textDim);
    setColour(juce::ComboBox::backgroundColourId,      BCol::knobBg);
    setColour(juce::ComboBox::textColourId,            BCol::text);
    setColour(juce::ComboBox::outlineColourId,         BCol::border);
    setColour(juce::ComboBox::arrowColourId,           BCol::accent);
    setColour(juce::PopupMenu::backgroundColourId,     BCol::panel);
    setColour(juce::PopupMenu::textColourId,           BCol::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, BCol::accent.withAlpha(0.4f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

void BombLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                        float pos, float startAngle, float endAngle,
                                        juce::Slider& s) {
    const float cx = x + w * 0.5f, cy = y + h * 0.5f;
    const float r  = std::min(w, h) * 0.36f;
    const float thick = r * 0.22f;
    const float valAngle = startAngle + pos * (endAngle - startAngle);

    juce::Path bg;
    bg.addCentredArc(cx, cy, r, r, 0.f, startAngle, endAngle, true);
    g.setColour(BCol::knobBg);
    g.strokePath(bg, juce::PathStrokeType(thick, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));

    juce::Colour arc = s.findColour(juce::Slider::rotarySliderFillColourId);
    juce::Path va;
    va.addCentredArc(cx, cy, r, r, 0.f, startAngle, valAngle, true);
    g.setColour(arc);
    g.strokePath(va, juce::PathStrokeType(thick, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));

    const float px = cx + r * std::sin(valAngle);
    const float py = cy - r * std::cos(valAngle);
    g.setColour(arc.withAlpha(0.5f));
    g.fillEllipse(px - 4.f, py - 4.f, 8.f, 8.f);
    g.setColour(arc);
    g.fillEllipse(px - 2.f, py - 2.f, 4.f, 4.f);
}

void BombLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool,
                                    int bx, int by, int bw, int bh, juce::ComboBox&) {
    g.setColour(BCol::knobBg);
    g.fillRoundedRectangle(0.f, 0.f, (float)w, (float)h, 5.f);
    g.setColour(BCol::border);
    g.drawRoundedRectangle(0.5f, 0.5f, w - 1.f, h - 1.f, 5.f, 1.f);
    float ax = bx + bw * 0.5f, ay = by + bh * 0.5f;
    juce::Path arrow;
    arrow.addTriangle(ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
    g.setColour(BCol::accent);
    g.fillPath(arrow);
}

void BombLookAndFeel::positionComboBoxText(juce::ComboBox& b, juce::Label& l) {
    l.setBounds(6, 0, b.getWidth() - 26, b.getHeight());
    l.setFont(juce::Font(11.f));
}

void BombLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& l) {
    g.fillAll(l.findColour(juce::Label::backgroundColourId));
    g.setColour(l.findColour(juce::Label::textColourId));
    g.setFont(l.getFont());
    g.drawFittedText(l.getText(), l.getLocalBounds(), l.getJustificationType(), 1, 1.f);
}

// ─── BKnob ───────────────────────────────────────────────────────────────────
BKnob::BKnob(const juce::String& name, juce::Colour arcColour) : arcColour_(arcColour) {
    slider.setSliderStyle(juce::Slider::Rotary);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 14);
    slider.setColour(juce::Slider::rotarySliderFillColourId, arcColour);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(9.f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, arcColour.withAlpha(0.85f));
    addAndMakeVisible(label);
}

void BKnob::resized() {
    auto b = getLocalBounds();
    label.setBounds(b.removeFromBottom(13));
    slider.setBounds(b);
}

// ─── SectionPanel ─────────────────────────────────────────────────────────────
SectionPanel::SectionPanel(const juce::String& title, juce::Colour hc)
    : title_(title), headerColour_(hc) {}

void SectionPanel::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();
    g.setColour(BCol::section);
    g.fillRoundedRectangle(b, 8.f);
    g.setColour(headerColour_.withAlpha(0.35f));
    g.drawRoundedRectangle(b.reduced(0.5f), 8.f, 1.f);
    juce::Path hdr;
    hdr.addRoundedRectangle(b.getX(), b.getY(), b.getWidth(), 20.f, 8.f, 8.f, true, true, false, false);
    g.setColour(headerColour_.withAlpha(0.15f));
    g.fillPath(hdr);
    g.setColour(headerColour_);
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText(title_, (int)b.getX() + 8, (int)b.getY(), (int)b.getWidth() - 16, 20,
               juce::Justification::centredLeft);
}

juce::Rectangle<int> SectionPanel::getContentArea() const {
    return getLocalBounds().reduced(6).withTrimmedTop(20);
}

// ─── OscStrip ─────────────────────────────────────────────────────────────────
OscStrip::OscStrip(int idx) : index_(idx) {
    addAndMakeVisible(display);
    for (auto* k : { &morphKnob, &tuneKnob, &fineKnob, &levelKnob,
                      &fmKnob, &unisonKnob, &detuneKnob })
        addAndMakeVisible(k);
    for (auto* k : { &densityKnob, &sizeKnob, &sprayKnob, &pitchScKnob }) {
        addAndMakeVisible(k);
        k->setVisible(false);
    }
    addAndMakeVisible(warpAmtKnob);

    // Mode button [WT|GR]
    modeBtn.setColour(juce::TextButton::buttonColourId,   BCol::knobBg);
    modeBtn.setColour(juce::TextButton::buttonOnColourId, BCol::green.withAlpha(0.3f));
    modeBtn.setColour(juce::TextButton::textColourOffId,  BCol::textDim);
    modeBtn.setColour(juce::TextButton::textColourOnId,   BCol::green);
    modeBtn.setClickingTogglesState(false);
    modeBtn.onClick = [this] {
        setGranMode(!granMode_);
        if (onModeChanged) onModeChanged(granMode_);
    };
    addAndMakeVisible(modeBtn);

    // Warp mode cycle button
    warpModeBtn.setColour(juce::TextButton::buttonColourId,  BCol::knobBg);
    warpModeBtn.setColour(juce::TextButton::textColourOffId, BCol::textDim);
    warpModeBtn.onClick = [this] {
        warpMode_ = (warpMode_ + 1) % 4;
        static const char* labels[] = { "WARP", "PB", "SM", "MR" };
        warpModeBtn.setButtonText(labels[warpMode_]);
        warpModeBtn.setColour(juce::TextButton::textColourOffId,
                              warpMode_ == 0 ? BCol::textDim : juce::Colour(0xFFFF9800u));
        display.setWarpMode(warpMode_);
        if (onWarpModeChanged) onWarpModeChanged(warpMode_);
    };
    addAndMakeVisible(warpModeBtn);

    morphKnob.slider.addListener(this);
}

OscStrip::~OscStrip() {
    morphKnob.slider.removeListener(this);
}

void OscStrip::setGranMode(bool gran) {
    granMode_ = gran;
    modeBtn.setButtonText(gran ? "GR" : "WT");
    modeBtn.setToggleState(gran, juce::dontSendNotification);
    morphKnob.label.setText(gran ? "POS" : "MORPH", juce::dontSendNotification);

    for (auto* k : { &tuneKnob, &fineKnob, &fmKnob, &unisonKnob, &detuneKnob })
        k->setVisible(!gran);
    for (auto* k : { &densityKnob, &sizeKnob, &sprayKnob, &pitchScKnob })
        k->setVisible(gran);
    resized();
}

void OscStrip::setWarpModeDisplay(int mode) {
    warpMode_ = mode;
    static const char* labels[] = { "WARP", "PB", "SM", "MR" };
    warpModeBtn.setButtonText(labels[mode]);
    warpModeBtn.setColour(juce::TextButton::textColourOffId,
                          mode == 0 ? BCol::textDim : juce::Colour(0xFFFF9800u));
    display.setWarpMode(mode);
}

void OscStrip::sliderValueChanged(juce::Slider* s) {
    if (s == &morphKnob.slider)
        display.setMorphPos((float)morphKnob.slider.getValue());
}

void OscStrip::paint(juce::Graphics& g) {
    const int H = getHeight();
    const juce::Colour labelCol = BCol::accent2;

    // OSC badge — pill background (top 60% of label area)
    const int badgeH = (int)(H * 0.55f);
    juce::Rectangle<float> badge(2.f, 2.f, (float)kLabelW - 4.f, (float)badgeH - 4.f);
    g.setColour(labelCol.withAlpha(0.12f));
    g.fillRoundedRectangle(badge, 4.f);
    g.setColour(labelCol.withAlpha(0.4f));
    g.drawRoundedRectangle(badge, 4.f, 0.8f);

    // "OSC" text
    g.setFont(juce::Font(7.5f, juce::Font::bold));
    g.setColour(labelCol.withAlpha(0.6f));
    g.drawText("OSC", (int)badge.getX(), (int)badge.getY() + 2,
               (int)badge.getWidth(), 10, juce::Justification::centred);

    // Number
    g.setFont(juce::Font(14.f, juce::Font::bold));
    g.setColour(labelCol);
    g.drawText(juce::String(index_), (int)badge.getX(),
               (int)badge.getY() + 10, (int)badge.getWidth(),
               (int)badge.getHeight() - 10, juce::Justification::centred);

    // Row divider
    g.setColour(BCol::border);
    g.drawHorizontalLine(H - 1, 0.f, (float)getWidth());
}

void OscStrip::resized() {
    auto b = getLocalBounds().reduced(0, 2);
    const int H = b.getHeight();

    // OSC label + mode button on left
    auto labelArea = b.removeFromLeft(kLabelW);
    const int badgeH = (int)(H * 0.55f);
    modeBtn.setBounds(labelArea.removeFromBottom(H - badgeH - 2).reduced(1, 1));

    // Wavetable display
    display.setBounds(b.removeFromLeft(kDisplayW));
    b.removeFromLeft(4);

    // Warp controls: [WARP MODE btn] [AMT knob]
    warpModeBtn.setBounds(b.removeFromLeft(kWarpBtnW).reduced(0, H / 4));
    b.removeFromLeft(2);
    warpAmtKnob.setBounds(b.removeFromLeft(kWarpKnobW));
    b.removeFromLeft(4);

    // Remaining knob slots
    if (!granMode_) {
        BKnob* knobs[] = { &morphKnob, &tuneKnob, &fineKnob,
                            &levelKnob, &fmKnob, &unisonKnob, &detuneKnob };
        const int nk = 7;
        const int kw = (b.getWidth() - (nk - 1) * 4) / nk;
        for (auto* k : knobs) {
            k->setBounds(b.removeFromLeft(kw));
            b.removeFromLeft(4);
        }
    } else {
        BKnob* knobs[] = { &morphKnob, &densityKnob, &sizeKnob,
                            &sprayKnob, &pitchScKnob, &levelKnob };
        const int nk = 6;
        const int kw = (b.getWidth() - (nk - 1) * 4) / nk;
        for (auto* k : knobs) {
            k->setBounds(b.removeFromLeft(kw));
            b.removeFromLeft(4);
        }
    }
}

// ─── Main Editor ──────────────────────────────────────────────────────────────
BombSynthAudioProcessorEditor::BombSynthAudioProcessorEditor(BombSynthAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), proc_(p),
      sequencerPanel_(p.sequencer())
{
    modPanel_ = std::make_unique<ModulationPanel>(p.parameters());
    setLookAndFeel(&laf_);
    setSize(1100, 700);
    setResizable(true, true);

    int ls = 0;
    auto* ld = BinaryData::getNamedResource("logo_png", ls);
    if (ld && ls > 0) logo_ = juce::ImageFileFormat::loadFrom(ld, ls);

    auto& par = proc_.parameters();

    // ── OSC section panel first so strips render on top ──────────────────────
    addAndMakeVisible(oscSection_);

    // ── OSC attachments + display callbacks ──────────────────────────────────
    static const char* morphIds[3]   = {"osc1_morph","osc2_morph","osc3_morph"};
    static const char* tuneIds[3]    = {"osc1_tune", "osc2_tune", "osc3_tune" };
    static const char* fineIds[3]    = {"osc1_fine", "osc2_fine", "osc3_fine" };
    static const char* levelIds[3]   = {"osc1_level","osc2_level","osc3_level"};
    static const char* fmIds[3]      = {"osc1_fm",   "osc2_fm",   "osc3_fm"  };
    static const char* uniIds[3]     = {"osc1_uni",  "osc2_uni",  "osc3_uni" };
    static const char* detIds[3]     = {"osc1_detune","osc2_detune","osc3_detune"};
    static const char* bankIds[3]    = {"osc1_wave", "osc2_wave", "osc3_wave"};
    static const char* engineIds[3]  = {"osc1_engine","osc2_engine","osc3_engine"};
    static const char* granDenIds[3] = {"osc1_gran_density","osc2_gran_density","osc3_gran_density"};
    static const char* granSzIds[3]  = {"osc1_gran_size","osc2_gran_size","osc3_gran_size"};
    static const char* granSpIds[3]  = {"osc1_gran_spray","osc2_gran_spray","osc3_gran_spray"};
    static const char* granPiIds[3]  = {"osc1_gran_pitch","osc2_gran_pitch","osc3_gran_pitch"};
    static const char* warpAmtIds[3] = {"osc1_warp_amt","osc2_warp_amt","osc3_warp_amt"};
    static const char* warpMdIds[3]  = {"osc1_warp_mode","osc2_warp_mode","osc3_warp_mode"};

    for (int i = 0; i < 3; ++i) {
        oscMorphAtt_   [i] = std::make_unique<SA>(par, morphIds[i],  oscs_[i].morphKnob.slider);
        oscTuneAtt_    [i] = std::make_unique<SA>(par, tuneIds[i],   oscs_[i].tuneKnob.slider);
        oscFineAtt_    [i] = std::make_unique<SA>(par, fineIds[i],   oscs_[i].fineKnob.slider);
        oscLevelAtt_   [i] = std::make_unique<SA>(par, levelIds[i],  oscs_[i].levelKnob.slider);
        oscFmAtt_      [i] = std::make_unique<SA>(par, fmIds[i],     oscs_[i].fmKnob.slider);
        oscUniAtt_     [i] = std::make_unique<SA>(par, uniIds[i],    oscs_[i].unisonKnob.slider);
        oscDetuneAtt_  [i] = std::make_unique<SA>(par, detIds[i],    oscs_[i].detuneKnob.slider);
        oscGranDensAtt_[i] = std::make_unique<SA>(par, granDenIds[i],oscs_[i].densityKnob.slider);
        oscGranSizeAtt_[i] = std::make_unique<SA>(par, granSzIds[i], oscs_[i].sizeKnob.slider);
        oscGranSprayAtt_[i]= std::make_unique<SA>(par, granSpIds[i], oscs_[i].sprayKnob.slider);
        oscGranPitchAtt_[i]= std::make_unique<SA>(par, granPiIds[i], oscs_[i].pitchScKnob.slider);
        oscWarpAmtAtt_  [i]= std::make_unique<SA>(par, warpAmtIds[i],oscs_[i].warpAmtKnob.slider);

        // Restore engine mode from stored param
        bool isGran = ((int)par.getRawParameterValue(engineIds[i])->load() != 0);
        if (isGran) oscs_[i].setGranMode(true);

        // Restore warp mode display from stored param
        int storedWarpMode = (int)par.getRawParameterValue(warpMdIds[i])->load();
        if (storedWarpMode != 0) oscs_[i].setWarpModeDisplay(storedWarpMode);

        // Mode button → APVTS engine param
        const juce::String engParamId = engineIds[i];
        oscs_[i].onModeChanged = [this, engParamId](bool gran) {
            if (auto* param = proc_.parameters().getParameter(engParamId))
                param->setValueNotifyingHost(gran ? 1.f : 0.f);
        };

        // Warp mode button → APVTS warp_mode param
        const juce::String warpParamId = warpMdIds[i];
        oscs_[i].onWarpModeChanged = [this, warpParamId](int mode) {
            if (auto* param = proc_.parameters().getParameter(warpParamId)) {
                auto* rp = dynamic_cast<juce::RangedAudioParameter*>(param);
                if (rp) param->setValueNotifyingHost(rp->getNormalisableRange().convertTo0to1((float)mode));
            }
        };

        // Initialise display bank from stored param
        int storedBank = (int)par.getRawParameterValue(bankIds[i])->load();
        oscs_[i].display.setBankIndex(storedBank);
        oscs_[i].display.setWarpMode(storedWarpMode);

        // Wire display navigation → APVTS
        const juce::String bankParamId = bankIds[i];
        oscs_[i].display.onBankChanged = [this, bankParamId](int newBank) {
            if (auto* param = proc_.parameters().getParameter(bankParamId)) {
                auto& range = dynamic_cast<juce::RangedAudioParameter*>(param)->getNormalisableRange();
                param->setValueNotifyingHost(range.convertTo0to1((float)newBank));
            }
        };

        const int oscI = i;
        oscs_[i].display.onLoadWavetable = [this, oscI] {
            loadWavetableForOsc(oscI);
        };

        addAndMakeVisible(oscs_[i]);
    }

    // ── Filter ───────────────────────────────────────────────────────────────
    for (auto& item : { std::pair<int,const char*>{1,"Ladder LP24"}, {2,"Ladder LP12"},
                         {3,"Ladder HP"}, {4,"SVF LP"}, {5,"Formant"}, {6,"Comb"} })
        filterTypeBox_.addItem(item.second, item.first);
    filterTypeLabel_.setText("TYPE", juce::dontSendNotification);
    filterTypeLabel_.setFont(juce::Font(9.5f, juce::Font::bold));
    filterTypeLabel_.setColour(juce::Label::textColourId, BCol::amber.withAlpha(0.8f));
    filterTypeLabel_.setJustificationType(juce::Justification::centred);

    cutoffAtt_     = std::make_unique<SA>(par, "filter_cutoff",  cutoffKnob_.slider);
    resAtt_        = std::make_unique<SA>(par, "filter_res",      resKnob_.slider);
    driveAtt_      = std::make_unique<SA>(par, "filter_drive",    driveKnob_.slider);
    envAmtAtt_     = std::make_unique<SA>(par, "filter_env_amt",  envAmtKnob_.slider);
    filterTypeAtt_ = std::make_unique<IA>(par, "filter_type",     filterTypeBox_);

    addAndMakeVisible(filterSection_);
    addAndMakeVisible(filterTypeLabel_);
    addAndMakeVisible(filterTypeBox_);
    addAndMakeVisible(filterDisplay_);
    for (auto* k : { &cutoffKnob_, &resKnob_, &driveKnob_, &envAmtKnob_ })
        addAndMakeVisible(k);

    // ── Amp Envelope ─────────────────────────────────────────────────────────
    ampAttAtt_ = std::make_unique<SA>(par, "amp_attack",  ampAttKnob_.slider);
    ampDecAtt_ = std::make_unique<SA>(par, "amp_decay",   ampDecKnob_.slider);
    ampSusAtt_ = std::make_unique<SA>(par, "amp_sustain", ampSusKnob_.slider);
    ampRelAtt_ = std::make_unique<SA>(par, "amp_release", ampRelKnob_.slider);
    ampCrvAtt_ = std::make_unique<SA>(par, "amp_curve",   ampCrvKnob_.slider);

    addAndMakeVisible(ampEnvSection_);
    for (auto* k : { &ampAttKnob_, &ampDecKnob_, &ampSusKnob_, &ampRelKnob_, &ampCrvKnob_ })
        addAndMakeVisible(k);

    // ── Filter Envelope ───────────────────────────────────────────────────────
    fEnvAttAtt_ = std::make_unique<SA>(par, "fenv_attack",  fEnvAttKnob_.slider);
    fEnvDecAtt_ = std::make_unique<SA>(par, "fenv_decay",   fEnvDecKnob_.slider);
    fEnvSusAtt_ = std::make_unique<SA>(par, "fenv_sustain", fEnvSusKnob_.slider);
    fEnvRelAtt_ = std::make_unique<SA>(par, "fenv_release", fEnvRelKnob_.slider);

    addAndMakeVisible(filterEnvSection_);
    for (auto* k : { &fEnvAttKnob_, &fEnvDecKnob_, &fEnvSusKnob_, &fEnvRelKnob_ })
        addAndMakeVisible(k);

    // ── LFO 1 ────────────────────────────────────────────────────────────────
    for (auto& item : { std::pair<int,const char*>{1,"Sine"},{2,"Triangle"},{3,"Saw"},
                         {4,"Rev Saw"},{5,"Square"},{6,"S & H"},{7,"Smooth"} })
        lfo1ShapeBox_.addItem(item.second, item.first);
    lfo1ShapeLabel_.setText("SHAPE", juce::dontSendNotification);
    lfo1ShapeLabel_.setFont(juce::Font(9.5f, juce::Font::bold));
    lfo1ShapeLabel_.setColour(juce::Label::textColourId, BCol::accent.withAlpha(0.8f));
    lfo1ShapeLabel_.setJustificationType(juce::Justification::centred);

    lfo1RateAtt_  = std::make_unique<SA>(par, "lfo1_rate",  lfo1RateKnob_.slider);
    lfo1DepthAtt_ = std::make_unique<SA>(par, "lfo1_depth", lfo1DepthKnob_.slider);
    lfo1PhaseAtt_ = std::make_unique<SA>(par, "lfo1_phase", lfo1PhaseKnob_.slider);
    lfo1ShapeAtt_ = std::make_unique<IA>(par, "lfo1_shape", lfo1ShapeBox_);

    addAndMakeVisible(lfo1Section_);
    addAndMakeVisible(lfo1ShapeLabel_);
    addAndMakeVisible(lfo1ShapeBox_);
    for (auto* k : { &lfo1RateKnob_, &lfo1DepthKnob_, &lfo1PhaseKnob_ })
        addAndMakeVisible(k);

    // ── LFO 2 ────────────────────────────────────────────────────────────────
    for (auto& item : { std::pair<int,const char*>{1,"Sine"},{2,"Triangle"},{3,"Saw"},
                         {4,"Rev Saw"},{5,"Square"},{6,"S & H"},{7,"Smooth"} })
        lfo2ShapeBox_.addItem(item.second, item.first);
    lfo2ShapeLabel_.setText("SHAPE", juce::dontSendNotification);
    lfo2ShapeLabel_.setFont(juce::Font(9.5f, juce::Font::bold));
    lfo2ShapeLabel_.setColour(juce::Label::textColourId, BCol::accent.withAlpha(0.8f));
    lfo2ShapeLabel_.setJustificationType(juce::Justification::centred);

    lfo2RateAtt_  = std::make_unique<SA>(par, "lfo2_rate",  lfo2RateKnob_.slider);
    lfo2DepthAtt_ = std::make_unique<SA>(par, "lfo2_depth", lfo2DepthKnob_.slider);
    lfo2ShapeAtt_ = std::make_unique<IA>(par, "lfo2_shape", lfo2ShapeBox_);

    addAndMakeVisible(lfo2Section_);
    addAndMakeVisible(lfo2ShapeLabel_);
    addAndMakeVisible(lfo2ShapeBox_);
    for (auto* k : { &lfo2RateKnob_, &lfo2DepthKnob_ })
        addAndMakeVisible(k);

    // ── Master ───────────────────────────────────────────────────────────────
    masterVolAtt_ = std::make_unique<SA>(par, "master_gain", masterVolKnob_.slider);
    glideAtt_     = std::make_unique<SA>(par, "glide_time",  glideKnob_.slider);

    addAndMakeVisible(masterSection_);
    addAndMakeVisible(masterVolKnob_);
    addAndMakeVisible(glideKnob_);

    // ── Effects components ────────────────────────────────────────────────────
    for (auto* c : { &reverbSection_, &delaySection_, &chorusSection_ })
        addAndMakeVisible(c);
    for (auto* k : { &revRoomKnob_, &revDampKnob_, &revWidthKnob_, &revWetKnob_ })
        addAndMakeVisible(k);
    for (auto* k : { &delTimeKnob_, &delFbKnob_, &delWetKnob_ })
        addAndMakeVisible(k);
    for (auto* k : { &choRateKnob_, &choDepthKnob_, &choWetKnob_ })
        addAndMakeVisible(k);

    revRoomAtt_  = std::make_unique<SA>(par, "reverb_room",  revRoomKnob_.slider);
    revDampAtt_  = std::make_unique<SA>(par, "reverb_damp",  revDampKnob_.slider);
    revWidthAtt_ = std::make_unique<SA>(par, "reverb_width", revWidthKnob_.slider);
    revWetAtt_   = std::make_unique<SA>(par, "reverb_wet",   revWetKnob_.slider);
    delTimeAtt_  = std::make_unique<SA>(par, "delay_time",   delTimeKnob_.slider);
    delFbAtt_    = std::make_unique<SA>(par, "delay_fb",     delFbKnob_.slider);
    delWetAtt_   = std::make_unique<SA>(par, "delay_wet",    delWetKnob_.slider);
    choRateAtt_  = std::make_unique<SA>(par, "chorus_rate",  choRateKnob_.slider);
    choDepthAtt_ = std::make_unique<SA>(par, "chorus_depth", choDepthKnob_.slider);
    choWetAtt_   = std::make_unique<SA>(par, "chorus_wet",   choWetKnob_.slider);

    // ── Envelope displays ─────────────────────────────────────────────────────
    ampEnvDisplay_.colour = BCol::green;
    fEnvDisplay_.colour   = BCol::amber;
    addAndMakeVisible(ampEnvDisplay_);
    addAndMakeVisible(fEnvDisplay_);

    // ── Tab bar ───────────────────────────────────────────────────────────────
    for (auto* b : { &synthTabBtn_, &effectsTabBtn_, &modTabBtn_, &sequencerTabBtn_ }) {
        b->setColour(juce::TextButton::buttonColourId,    BCol::knobBg);
        b->setColour(juce::TextButton::buttonOnColourId,  BCol::accent.withAlpha(0.3f));
        b->setColour(juce::TextButton::textColourOffId,   BCol::textDim);
        b->setColour(juce::TextButton::textColourOnId,    BCol::accent);
        b->setClickingTogglesState(false);
        addAndMakeVisible(b);
    }
    synthTabBtn_.setToggleState(true, juce::dontSendNotification);
    synthTabBtn_.onClick      = [this] { setTab(Tab::Synth);       };
    effectsTabBtn_.onClick    = [this] { setTab(Tab::Effects);     };
    modTabBtn_.onClick        = [this] { setTab(Tab::Modulation);  };
    sequencerTabBtn_.onClick  = [this] { setTab(Tab::Sequencer);   };

    addAndMakeVisible(sequencerPanel_);
    sequencerPanel_.setVisible(false);

    addAndMakeVisible(*modPanel_);
    modPanel_->setVisible(false);

    setEffectsVisible(false);

    buildPresetBrowser();
    startTimerHz(20);
}

BombSynthAudioProcessorEditor::~BombSynthAudioProcessorEditor() {
    setLookAndFeel(nullptr);
}

// ─── Tab helpers ─────────────────────────────────────────────────────────────
void BombSynthAudioProcessorEditor::setSynthVisible(bool v) {
    for (auto& osc : oscs_) {
        osc.setVisible(v);
    }
    oscSection_.setVisible(v);
    filterSection_.setVisible(v);
    for (auto* k : { &cutoffKnob_, &resKnob_, &driveKnob_, &envAmtKnob_ }) k->setVisible(v);
    filterTypeBox_.setVisible(v);  filterTypeLabel_.setVisible(v);
    filterDisplay_.setVisible(v);
    ampEnvSection_.setVisible(v);
    for (auto* k : { &ampAttKnob_, &ampDecKnob_, &ampSusKnob_, &ampRelKnob_, &ampCrvKnob_ }) k->setVisible(v);
    filterEnvSection_.setVisible(v);
    for (auto* k : { &fEnvAttKnob_, &fEnvDecKnob_, &fEnvSusKnob_, &fEnvRelKnob_ }) k->setVisible(v);
    lfo1Section_.setVisible(v); lfo1ShapeBox_.setVisible(v); lfo1ShapeLabel_.setVisible(v);
    for (auto* k : { &lfo1RateKnob_, &lfo1DepthKnob_, &lfo1PhaseKnob_ }) k->setVisible(v);
    lfo2Section_.setVisible(v); lfo2ShapeBox_.setVisible(v); lfo2ShapeLabel_.setVisible(v);
    for (auto* k : { &lfo2RateKnob_, &lfo2DepthKnob_ }) k->setVisible(v);
    masterSection_.setVisible(v);
    masterVolKnob_.setVisible(v); glideKnob_.setVisible(v);
    ampEnvDisplay_.setVisible(v);  fEnvDisplay_.setVisible(v);
}

void BombSynthAudioProcessorEditor::setSequencerVisible(bool v) {
    sequencerPanel_.setVisible(v);
}

void BombSynthAudioProcessorEditor::setModulationVisible(bool v) {
    if (modPanel_) modPanel_->setVisible(v);
}

void BombSynthAudioProcessorEditor::setEffectsVisible(bool v) {
    reverbSection_.setVisible(v);
    for (auto* k : { &revRoomKnob_, &revDampKnob_, &revWidthKnob_, &revWetKnob_ }) k->setVisible(v);
    delaySection_.setVisible(v);
    for (auto* k : { &delTimeKnob_, &delFbKnob_, &delWetKnob_ }) k->setVisible(v);
    chorusSection_.setVisible(v);
    for (auto* k : { &choRateKnob_, &choDepthKnob_, &choWetKnob_ }) k->setVisible(v);
}

void BombSynthAudioProcessorEditor::setTab(Tab t) {
    activeTab_ = t;
    synthTabBtn_     .setToggleState(t == Tab::Synth,       juce::dontSendNotification);
    effectsTabBtn_   .setToggleState(t == Tab::Effects,     juce::dontSendNotification);
    modTabBtn_       .setToggleState(t == Tab::Modulation,  juce::dontSendNotification);
    sequencerTabBtn_ .setToggleState(t == Tab::Sequencer,   juce::dontSendNotification);
    setSynthVisible    (t == Tab::Synth);
    setEffectsVisible  (t == Tab::Effects);
    setModulationVisible(t == Tab::Modulation);
    setSequencerVisible(t == Tab::Sequencer);
    resized();
    repaint();
}

void BombSynthAudioProcessorEditor::timerCallback() {
    auto& par = proc_.parameters();
    auto getF = [&](const char* id) { return par.getRawParameterValue(id)->load(); };
    auto getI = [&](const char* id) { return (int)par.getRawParameterValue(id)->load(); };
    auto norm = [](float ms) { return std::pow(ms / 10000.f, 0.25f); };
    ampEnvDisplay_.setADSR(norm(getF("amp_attack")),  norm(getF("amp_decay")),
                            getF("amp_sustain"),       norm(getF("amp_release")));
    fEnvDisplay_.setADSR  (norm(getF("fenv_attack")), norm(getF("fenv_decay")),
                            getF("fenv_sustain"),      norm(getF("fenv_release")));
    filterDisplay_.setParams(getI("filter_type"), getF("filter_cutoff"), getF("filter_res"));

    // Sync warp mode display + engine mode from params (handles preset changes)
    static const char* warpMdIds[3]  = {"osc1_warp_mode","osc2_warp_mode","osc3_warp_mode"};
    static const char* warpAmtIds[3] = {"osc1_warp_amt","osc2_warp_amt","osc3_warp_amt"};
    static const char* engineIds[3]  = {"osc1_engine","osc2_engine","osc3_engine"};
    for (int i = 0; i < 3; ++i) {
        int wm = getI(warpMdIds[i]);
        if (wm != oscs_[i].warpModeVal())   oscs_[i].setWarpModeDisplay(wm);
        oscs_[i].display.setWarpAmt(getF(warpAmtIds[i]));
        bool gran = (getI(engineIds[i]) != 0);
        if (gran != oscs_[i].granMode())    oscs_[i].setGranMode(gran);
    }
}

// ─── Preset browser helpers ───────────────────────────────────────────────────
void BombSynthAudioProcessorEditor::buildPresetBrowser() {
    // Category combo
    categoryCombo_.clear(juce::dontSendNotification);
    auto cats = PresetManager::categories();
    for (int i = 0; i < cats.size(); ++i)
        categoryCombo_.addItem(cats[i], i + 1);
    categoryCombo_.setSelectedId(1, juce::dontSendNotification);

    categoryCombo_.onChange = [this] {
        populatePresetCombo(categoryCombo_.getText());
    };
    presetCombo_.onChange = [this] {
        int idx = PresetManager::indexOf(categoryCombo_.getText(), presetCombo_.getText());
        applyPreset(idx);
    };
    prevBtn_.onClick = [this] {
        applyPreset(PresetManager::prev(currentPresetIdx_));
    };
    nextBtn_.onClick = [this] {
        applyPreset(PresetManager::next(currentPresetIdx_));
    };

    addAndMakeVisible(categoryCombo_);
    addAndMakeVisible(presetCombo_);
    addAndMakeVisible(prevBtn_);
    addAndMakeVisible(nextBtn_);

    populatePresetCombo(cats[0]);

    // Style
    for (auto* c : { &categoryCombo_, &presetCombo_ }) {
        c->setColour(juce::ComboBox::backgroundColourId, BCol::knobBg);
        c->setColour(juce::ComboBox::textColourId, BCol::text);
        c->setColour(juce::ComboBox::outlineColourId, BCol::accent.withAlpha(0.4f));
        c->setColour(juce::ComboBox::arrowColourId, BCol::accent);
        c->setColour(juce::PopupMenu::backgroundColourId, BCol::panel);
        c->setColour(juce::PopupMenu::textColourId, BCol::text);
    }
    for (auto* b : { &prevBtn_, &nextBtn_ }) {
        b->setColour(juce::TextButton::buttonColourId,   BCol::knobBg);
        b->setColour(juce::TextButton::textColourOffId,  BCol::accent);
        b->setColour(juce::ComboBox::outlineColourId,    BCol::accent.withAlpha(0.4f));
    }
}

void BombSynthAudioProcessorEditor::populatePresetCombo(const juce::String& category) {
    presetCombo_.clear(juce::dontSendNotification);
    auto names = PresetManager::presetsInCategory(category);
    for (int i = 0; i < names.size(); ++i)
        presetCombo_.addItem(names[i], i + 1);
    presetCombo_.setSelectedItemIndex(0, juce::dontSendNotification);
}

void BombSynthAudioProcessorEditor::applyPreset(int idx) {
    currentPresetIdx_ = idx;
    PresetManager::apply(idx, proc_.parameters());

    // Sync combo selections to loaded preset
    const auto& preset = PresetManager::all()[idx];
    categoryCombo_.setText(preset.category, juce::dontSendNotification);
    populatePresetCombo(preset.category);
    presetCombo_.setText(preset.name, juce::dontSendNotification);

    // Update WavetableDisplay bank selections from newly loaded params
    syncDisplaysToParams();
}

void BombSynthAudioProcessorEditor::loadWavetableForOsc(int oscIdx) {
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Load Wavetable (WAV / AIFF)",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.aif;*.aiff");

    fileChooser_->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, oscIdx](const juce::FileChooser& fc) {
            static const char* bankIds[] = { "osc1_wave", "osc2_wave", "osc3_wave" };
            auto f = fc.getResult();
            if (!f.existsAsFile()) return;

            // Read audio into a mono float buffer
            juce::AudioFormatManager mgr;
            mgr.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader(mgr.createReaderFor(f));
            if (!reader) return;

            const int total = (int)reader->lengthInSamples;
            if (total < 2) return;

            juce::AudioBuffer<float> buf(1, total);
            reader->read(&buf, 0, total, 0, true, true);  // downmix stereo → mono

            // Pick a colour from the cycling palette
            uint32_t col = kUserColours[userColourIdx_++ % juce::numElementsInArray(kUserColours)];

            int bankIdx = WavetableBankLibrary::get().addUserBank(
                f.getFileNameWithoutExtension(), col,
                buf.getReadPointer(0), total,
                f.getFullPathName());

            if (bankIdx < 0) return;   // error

            // Persist the path
            proc_.addUserWavetablePath(f.getFullPathName());

            // Set param & display
            if (auto* param = proc_.parameters().getParameter(bankIds[oscIdx])) {
                auto* rp = dynamic_cast<juce::RangedAudioParameter*>(param);
                if (rp) param->setValueNotifyingHost(
                    rp->getNormalisableRange().convertTo0to1((float)bankIdx));
            }
            oscs_[oscIdx].display.setBankIndex(bankIdx);
        });
}

void BombSynthAudioProcessorEditor::syncDisplaysToParams() {
    static const char* bankIds[3] = {"osc1_wave","osc2_wave","osc3_wave"};
    static const char* morphIds[3] = {"osc1_morph","osc2_morph","osc3_morph"};
    for (int i = 0; i < 3; ++i) {
        int bank  = (int)proc_.parameters().getRawParameterValue(bankIds[i])->load();
        float mor =       proc_.parameters().getRawParameterValue(morphIds[i])->load();
        oscs_[i].display.setBankIndex(bank);
        oscs_[i].display.setMorphPos(mor);
    }
}

void BombSynthAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(BCol::bg);

    const int hH = 50;
    g.setColour(BCol::panel);
    g.fillRect(0, 0, getWidth(), hH);
    g.setColour(BCol::accent.withAlpha(0.6f));
    g.drawHorizontalLine(hH, 0.f, (float)getWidth());

    if (logo_.isValid()) {
        const int ls = 34, lx = getWidth() - ls - 14, ly = (hH - ls) / 2;
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.fillEllipse((float)lx, (float)ly, (float)ls, (float)ls);
        g.drawImage(logo_, lx, ly, ls, ls, 0, 0, logo_.getWidth(), logo_.getHeight());
    }

    g.setFont(juce::Font(22.f, juce::Font::bold));
    g.setColour(BCol::accent);
    g.drawText("BOMB SYNTH", 14, 0, 320, hH, juce::Justification::centredLeft);

    g.setFont(juce::Font(10.f));
    g.setColour(BCol::textDim);
    g.drawText("v0.6.0  |  Illbomb", getWidth() - 200, 0, 140, hH,
               juce::Justification::centredRight);
}

void BombSynthAudioProcessorEditor::resized() {
    const int hH  = 50;
    const int pad = 8;
    const int W   = getWidth();

    // ── Preset browser (inside header bar) ───────────────────────────────────
    {
        const int catW  = 90;
        const int preW  = 190;
        const int btnW  = 26;
        const int gap   = 5;
        const int barH  = 26;
        const int barY  = (hH - barH) / 2;
        // Centred in the middle of the header
        int totalW = catW + gap + preW + gap + btnW + gap + btnW;
        int startX = (W - totalW) / 2;
        categoryCombo_.setBounds(startX, barY, catW, barH);
        startX += catW + gap;
        presetCombo_   .setBounds(startX, barY, preW, barH);
        startX += preW + gap;
        prevBtn_       .setBounds(startX, barY, btnW, barH);
        startX += btnW + gap;
        nextBtn_       .setBounds(startX, barY, btnW, barH);
    }

    // ── Tab bar ───────────────────────────────────────────────────────────────
    {
        const int tabH = 28, tabW = 90, seqTabW = 110, gap = 4;
        int tx = pad, ty = hH + 2;
        synthTabBtn_     .setBounds(tx,                         ty, tabW,    tabH);
        effectsTabBtn_   .setBounds(tx + tabW + gap,            ty, tabW,    tabH);
        modTabBtn_       .setBounds(tx + (tabW + gap) * 2,      ty, tabW,    tabH);
        sequencerTabBtn_ .setBounds(tx + (tabW + gap) * 3,      ty, seqTabW, tabH);
    }

    const int kTabBarH = 32;
    auto area = getLocalBounds().withTrimmedTop(hH + kTabBarH).reduced(pad);
    const int aW = area.getWidth(); (void)aW;

    if (activeTab_ == Tab::Sequencer) {
        sequencerPanel_.setBounds(area);
        return;
    }

    if (activeTab_ == Tab::Modulation) {
        if (modPanel_) modPanel_->setBounds(area);
        return;
    }

    if (activeTab_ == Tab::Effects) {
        // ── EFFECTS tab layout ────────────────────────────────────────────────
        const int sectionW = (area.getWidth() - pad * 2) / 3;
        auto row = area;

        // REVERB
        auto revBounds = row.removeFromLeft(sectionW);
        reverbSection_.setBounds(revBounds);
        auto revContent = reverbSection_.getContentArea().translated(revBounds.getX(), revBounds.getY());
        int kw = (revContent.getWidth() - pad * 3) / 4;
        auto rc = revContent;
        revRoomKnob_ .setBounds(rc.removeFromLeft(kw)); rc.removeFromLeft(pad);
        revDampKnob_ .setBounds(rc.removeFromLeft(kw)); rc.removeFromLeft(pad);
        revWidthKnob_.setBounds(rc.removeFromLeft(kw)); rc.removeFromLeft(pad);
        revWetKnob_  .setBounds(rc.removeFromLeft(kw));
        row.removeFromLeft(pad);

        // DELAY
        auto delBounds = row.removeFromLeft(sectionW);
        delaySection_.setBounds(delBounds);
        auto delContent = delaySection_.getContentArea().translated(delBounds.getX(), delBounds.getY());
        int dw = (delContent.getWidth() - pad * 2) / 3;
        auto dc = delContent;
        delTimeKnob_.setBounds(dc.removeFromLeft(dw)); dc.removeFromLeft(pad);
        delFbKnob_  .setBounds(dc.removeFromLeft(dw)); dc.removeFromLeft(pad);
        delWetKnob_ .setBounds(dc.removeFromLeft(dw));
        row.removeFromLeft(pad);

        // CHORUS
        auto choBounds = row;
        chorusSection_.setBounds(choBounds);
        auto choContent = chorusSection_.getContentArea().translated(choBounds.getX(), choBounds.getY());
        int cw = (choContent.getWidth() - pad * 2) / 3;
        auto cc = choContent;
        choRateKnob_ .setBounds(cc.removeFromLeft(cw)); cc.removeFromLeft(pad);
        choDepthKnob_.setBounds(cc.removeFromLeft(cw)); cc.removeFromLeft(pad);
        choWetKnob_  .setBounds(cc.removeFromLeft(cw));
        return;
    }

    // ── SYNTH tab layout (existing) ───────────────────────────────────────────

    // ── Row 1: Oscillators ────────────────────────────────────────────────────
    auto row1 = area.removeFromTop(220);
    oscSection_.setBounds(row1);
    auto oscContent = oscSection_.getContentArea().translated(row1.getX(), row1.getY());
    const int oscStripH = oscContent.getHeight() / 3;
    for (auto& osc : oscs_)
        osc.setBounds(oscContent.removeFromTop(oscStripH));

    area.removeFromTop(pad);

    // ── Row 2: Filter  |  Amp Env  |  Filter Env ──────────────────────────────
    auto row2 = area.removeFromTop(220);

    // Filter (27%)
    int filterW = (int)(area.getWidth() * 0.27f);
    auto filterB = row2.removeFromLeft(filterW);
    filterSection_.setBounds(filterB);
    auto fc = filterSection_.getContentArea().translated(filterB.getX(), filterB.getY());
    auto typeRow = fc.removeFromTop(36);
    filterTypeLabel_.setBounds(typeRow.removeFromTop(14));
    filterTypeBox_  .setBounds(typeRow.withTrimmedRight(4).withTrimmedLeft(4));
    filterDisplay_  .setBounds(fc.removeFromTop(52).reduced(2, 0));
    fc.removeFromTop(2);
    const int fkW = fc.getWidth() / 4;
    for (auto* k : { &cutoffKnob_, &resKnob_, &driveKnob_, &envAmtKnob_ })
        k->setBounds(fc.removeFromLeft(fkW));

    row2.removeFromLeft(pad);

    // Amp env (36%)
    int ampW = (int)(area.getWidth() * 0.355f);
    auto ampB = row2.removeFromLeft(ampW);
    ampEnvSection_.setBounds(ampB);
    {
        auto ac = ampEnvSection_.getContentArea().translated(ampB.getX(), ampB.getY());
        // Envelope display strip (top 42px)
        ampEnvDisplay_.setBounds(ac.removeFromTop(42));
        ac.removeFromTop(2);
        const int akW = ac.getWidth() / 5;
        for (auto* k : { &ampAttKnob_, &ampDecKnob_, &ampSusKnob_, &ampRelKnob_, &ampCrvKnob_ })
            k->setBounds(ac.removeFromLeft(akW));
    }

    row2.removeFromLeft(pad);

    // Filter env (remainder)
    filterEnvSection_.setBounds(row2);
    {
        auto fec = filterEnvSection_.getContentArea().translated(row2.getX(), row2.getY());
        // Envelope display strip (top 42px)
        fEnvDisplay_.setBounds(fec.removeFromTop(42));
        fec.removeFromTop(2);
        const int fekW = fec.getWidth() / 4;
        for (auto* k : { &fEnvAttKnob_, &fEnvDecKnob_, &fEnvSusKnob_, &fEnvRelKnob_ })
            k->setBounds(fec.removeFromLeft(fekW));
    }

    area.removeFromTop(pad);

    // ── Row 3: LFO 1  |  LFO 2  |  Master ────────────────────────────────────
    auto row3 = area.removeFromTop(145);
    const int lfoW = (int)(area.getWidth() * 0.37f);

    // LFO 1
    auto lfo1B = row3.removeFromLeft(lfoW);
    lfo1Section_.setBounds(lfo1B);
    auto l1c = lfo1Section_.getContentArea().translated(lfo1B.getX(), lfo1B.getY());
    auto l1top = l1c.removeFromTop(38);
    lfo1ShapeLabel_.setBounds(l1top.removeFromTop(14));
    lfo1ShapeBox_  .setBounds(l1top.withTrimmedRight(4).withTrimmedLeft(4));
    const int l1kW = l1c.getWidth() / 3;
    for (auto* k : { &lfo1RateKnob_, &lfo1DepthKnob_, &lfo1PhaseKnob_ })
        k->setBounds(l1c.removeFromLeft(l1kW));

    row3.removeFromLeft(pad);

    // LFO 2
    auto lfo2B = row3.removeFromLeft(lfoW);
    lfo2Section_.setBounds(lfo2B);
    auto l2c = lfo2Section_.getContentArea().translated(lfo2B.getX(), lfo2B.getY());
    auto l2top = l2c.removeFromTop(38);
    lfo2ShapeLabel_.setBounds(l2top.removeFromTop(14));
    lfo2ShapeBox_  .setBounds(l2top.withTrimmedRight(4).withTrimmedLeft(4));
    const int l2kW = l2c.getWidth() / 2;
    for (auto* k : { &lfo2RateKnob_, &lfo2DepthKnob_ })
        k->setBounds(l2c.removeFromLeft(l2kW));

    row3.removeFromLeft(pad);

    // Master
    masterSection_.setBounds(row3);
    auto mc = masterSection_.getContentArea().translated(row3.getX(), row3.getY());
    const int mkW = mc.getWidth() / 2;
    masterVolKnob_.setBounds(mc.removeFromLeft(mkW));
    glideKnob_    .setBounds(mc.removeFromLeft(mkW));
}

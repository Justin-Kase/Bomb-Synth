#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <cmath>

// ─── BombLookAndFeel ─────────────────────────────────────────────────────────
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

    // Background ring
    juce::Path bg;
    bg.addCentredArc(cx, cy, r, r, 0.f, startAngle, endAngle, true);
    g.setColour(BCol::knobBg);
    g.strokePath(bg, juce::PathStrokeType(thick, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));

    // Value arc — colour from slider's custom property or fall back to accent
    juce::Colour arc = s.findColour(juce::Slider::rotarySliderFillColourId);
    juce::Path va;
    va.addCentredArc(cx, cy, r, r, 0.f, startAngle, valAngle, true);
    g.setColour(arc);
    g.strokePath(va, juce::PathStrokeType(thick, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));

    // Glow dot at tip
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
    // Arrow
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
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
    slider.setColour(juce::Slider::rotarySliderFillColourId, arcColour);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(9.5f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, arcColour.withAlpha(0.85f));
    addAndMakeVisible(label);
}

void BKnob::setArcColour(juce::Colour c) {
    arcColour_ = c;
    slider.setColour(juce::Slider::rotarySliderFillColourId, c);
    label.setColour(juce::Label::textColourId, c.withAlpha(0.85f));
}

void BKnob::resized() {
    auto b = getLocalBounds();
    label.setBounds(b.removeFromBottom(14));
    slider.setBounds(b);
}

// ─── SectionPanel ────────────────────────────────────────────────────────────
SectionPanel::SectionPanel(const juce::String& title, juce::Colour hc)
    : title_(title), headerColour_(hc) {}

void SectionPanel::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();
    // Background
    g.setColour(BCol::section);
    g.fillRoundedRectangle(b, 8.f);
    // Border
    g.setColour(headerColour_.withAlpha(0.35f));
    g.drawRoundedRectangle(b.reduced(0.5f), 8.f, 1.f);
    // Header bar
    juce::Path header;
    header.addRoundedRectangle(b.getX(), b.getY(), b.getWidth(), 20.f, 8.f, 8.f, true, true, false, false);
    g.setColour(headerColour_.withAlpha(0.15f));
    g.fillPath(header);
    // Title text
    g.setColour(headerColour_);
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText(title_, (int)b.getX() + 8, (int)b.getY(), (int)b.getWidth() - 16, 20,
               juce::Justification::centredLeft);
}

juce::Rectangle<int> SectionPanel::getContentArea() const {
    return getLocalBounds().reduced(6).withTrimmedTop(20);
}

// ─── OscStrip ────────────────────────────────────────────────────────────────
OscStrip::OscStrip(int idx) : index_(idx) {
    oscLabel.setText("OSC " + juce::String(idx), juce::dontSendNotification);
    oscLabel.setFont(juce::Font(11.f, juce::Font::bold));
    oscLabel.setColour(juce::Label::textColourId, BCol::accent2);
    oscLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(oscLabel);

    for (auto& item : { std::pair<int,const char*>{1,"Sine"}, {2,"Saw"}, {3,"Square"},
                         {4,"Triangle"}, {5,"Saw+Tri"} })
        waveBox.addItem(item.second, item.first);
    waveBox.setSelectedId(2, juce::dontSendNotification);
    addAndMakeVisible(waveBox);

    for (auto* k : { &tuneKnob, &fineKnob, &levelKnob, &fmKnob, &unisonKnob, &detuneKnob })
        addAndMakeVisible(k);
}

void OscStrip::paint(juce::Graphics& g) {
    // Subtle divider line at bottom
    g.setColour(BCol::border);
    g.drawHorizontalLine(getHeight() - 1, 0.f, (float)getWidth());
}

void OscStrip::resized() {
    auto b = getLocalBounds().reduced(2, 0);
    // Label (left)
    oscLabel.setBounds(b.removeFromLeft(42));
    // Waveform combo
    waveBox.setBounds(b.removeFromLeft(80).withSizeKeepingCentre(78, 22));
    b.removeFromLeft(4);
    // Knobs
    const int kw = (b.getWidth() - 20) / 6;
    for (auto* k : { &tuneKnob, &fineKnob, &levelKnob, &fmKnob, &unisonKnob, &detuneKnob }) {
        k->setBounds(b.removeFromLeft(kw));
        b.removeFromLeft(4);
    }
}

// ─── Main Editor ─────────────────────────────────────────────────────────────
BombSynthAudioProcessorEditor::BombSynthAudioProcessorEditor(BombSynthAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), proc_(p)
{
    setLookAndFeel(&laf_);
    setSize(1100, 680);
    setResizable(true, true);

    // Logo
    int ls = 0;
    auto* ld = BinaryData::getNamedResource("logo_png", ls);
    if (ld && ls > 0) logo_ = juce::ImageFileFormat::loadFrom(ld, ls);

    auto& par = proc_.parameters();

    // ── OSC attachments ──────────────────────────────────────────────────────
    static const char* oscWaveIds[3]   = {"osc1_wave","osc2_wave","osc3_wave"};
    static const char* oscTuneIds[3]   = {"osc1_tune","osc2_tune","osc3_tune"};
    static const char* oscFineIds[3]   = {"osc1_fine","osc2_fine","osc3_fine"};
    static const char* oscLevelIds[3]  = {"osc1_level","osc2_level","osc3_level"};
    static const char* oscFmIds[3]     = {"osc1_fm","osc2_fm","osc3_fm"};
    static const char* oscUniIds[3]    = {"osc1_uni","osc2_uni","osc3_uni"};
    static const char* oscDetuneIds[3] = {"osc1_detune","osc2_detune","osc3_detune"};

    for (int i = 0; i < 3; ++i) {
        oscWaveAtt_  [i] = std::make_unique<IA>(par, oscWaveIds[i],   oscs_[i].waveBox);
        oscTuneAtt_  [i] = std::make_unique<SA>(par, oscTuneIds[i],   oscs_[i].tuneKnob.slider);
        oscFineAtt_  [i] = std::make_unique<SA>(par, oscFineIds[i],   oscs_[i].fineKnob.slider);
        oscLevelAtt_ [i] = std::make_unique<SA>(par, oscLevelIds[i],  oscs_[i].levelKnob.slider);
        oscFmAtt_    [i] = std::make_unique<SA>(par, oscFmIds[i],     oscs_[i].fmKnob.slider);
        oscUniAtt_   [i] = std::make_unique<SA>(par, oscUniIds[i],    oscs_[i].unisonKnob.slider);
        oscDetuneAtt_[i] = std::make_unique<SA>(par, oscDetuneIds[i], oscs_[i].detuneKnob.slider);
        addAndMakeVisible(oscs_[i]);
    }
    addAndMakeVisible(oscSection_);

    // ── Filter ───────────────────────────────────────────────────────────────
    for (auto& item : { std::pair<int,const char*>{1,"Ladder LP24"}, {2,"Ladder LP12"},
                         {3,"Ladder HP24"}, {4,"SVF LP"}, {5,"SVF BP"} })
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
    for (auto& item : { std::pair<int,const char*>{1,"Sine"}, {2,"Triangle"}, {3,"Saw"},
                         {4,"Rev Saw"}, {5,"Square"}, {6,"S & H"}, {7,"Smooth Rand"} })
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
    for (auto& item : { std::pair<int,const char*>{1,"Sine"}, {2,"Triangle"}, {3,"Saw"},
                         {4,"Rev Saw"}, {5,"Square"}, {6,"S & H"}, {7,"Smooth Rand"} })
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
}

BombSynthAudioProcessorEditor::~BombSynthAudioProcessorEditor() {
    setLookAndFeel(nullptr);
}

void BombSynthAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(BCol::bg);

    // ── Header bar ───────────────────────────────────────────────────────────
    const int hH = 50;
    g.setColour(BCol::panel);
    g.fillRect(0, 0, getWidth(), hH);
    g.setColour(BCol::accent.withAlpha(0.6f));
    g.drawHorizontalLine(hH, 0.f, (float)getWidth());

    // Logo
    if (logo_.isValid()) {
        const int ls = 34, lx = getWidth() - ls - 14, ly = (hH - ls) / 2;
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.fillEllipse((float)lx, (float)ly, (float)ls, (float)ls);
        g.drawImage(logo_, lx, ly, ls, ls, 0, 0, logo_.getWidth(), logo_.getHeight());
    }

    // Title
    g.setFont(juce::Font(22.f, juce::Font::bold));
    g.setColour(BCol::accent);
    g.drawText("BOMB SYNTH", 14, 0, 320, hH, juce::Justification::centredLeft);

    // Subtitle
    g.setFont(juce::Font(10.f));
    g.setColour(BCol::textDim);
    g.drawText("Hybrid Synthesizer  |  v0.1.0  |  Illbomb",
               14, 0, getWidth() - 60, hH, juce::Justification::centredRight);
}

void BombSynthAudioProcessorEditor::resized() {
    const int hH  = 50;   // header
    const int pad = 8;
    auto area = getLocalBounds().withTrimmedTop(hH).reduced(pad);
    const int W = area.getWidth();

    // ── Row 1: Oscillators (full width) ─────────────────────────────────────
    auto row1 = area.removeFromTop(185);
    oscSection_.setBounds(row1);
    auto oscContent = oscSection_.getContentArea().translated(row1.getX(), row1.getY());
    const int oscH = oscContent.getHeight() / 3;
    for (auto& osc : oscs_)
        osc.setBounds(oscContent.removeFromTop(oscH));

    area.removeFromTop(pad);

    // ── Row 2: Filter + Amp Env + Filter Env ────────────────────────────────
    auto row2 = area.removeFromTop(160);

    // Filter (left, 30% width)
    int filterW = (int)(W * 0.27f);
    auto filterBounds = row2.removeFromLeft(filterW);
    filterSection_.setBounds(filterBounds);
    auto fc = filterSection_.getContentArea().translated(filterBounds.getX(), filterBounds.getY());
    // Type combo top
    auto typeRow = fc.removeFromTop(32);
    filterTypeLabel_.setBounds(typeRow.removeFromLeft(44));
    filterTypeBox_  .setBounds(typeRow.withTrimmedRight(4));
    // Knobs below
    const int fkW = fc.getWidth() / 4;
    for (auto* k : { &cutoffKnob_, &resKnob_, &driveKnob_, &envAmtKnob_ }) {
        k->setBounds(fc.removeFromLeft(fkW));
    }

    row2.removeFromLeft(pad);

    // Amp env (middle, ~36%)
    int ampW = (int)(W * 0.355f);
    auto ampBounds = row2.removeFromLeft(ampW);
    ampEnvSection_.setBounds(ampBounds);
    auto ac = ampEnvSection_.getContentArea().translated(ampBounds.getX(), ampBounds.getY());
    const int akW = ac.getWidth() / 5;
    for (auto* k : { &ampAttKnob_, &ampDecKnob_, &ampSusKnob_, &ampRelKnob_, &ampCrvKnob_ })
        k->setBounds(ac.removeFromLeft(akW));

    row2.removeFromLeft(pad);

    // Filter env (right, remainder)
    auto fenvBounds = row2;
    filterEnvSection_.setBounds(fenvBounds);
    auto fec = filterEnvSection_.getContentArea().translated(fenvBounds.getX(), fenvBounds.getY());
    const int fekW = fec.getWidth() / 4;
    for (auto* k : { &fEnvAttKnob_, &fEnvDecKnob_, &fEnvSusKnob_, &fEnvRelKnob_ })
        k->setBounds(fec.removeFromLeft(fekW));

    area.removeFromTop(pad);

    // ── Row 3: LFO 1 + LFO 2 + Master ──────────────────────────────────────
    auto row3 = area.removeFromTop(140);
    int lfoW = (int)(W * 0.37f);
    int masterW = W - lfoW * 2 - pad * 2;

    // LFO 1
    auto lfo1Bounds = row3.removeFromLeft(lfoW);
    lfo1Section_.setBounds(lfo1Bounds);
    auto l1c = lfo1Section_.getContentArea().translated(lfo1Bounds.getX(), lfo1Bounds.getY());
    auto l1top = l1c.removeFromTop(34);
    lfo1ShapeLabel_.setBounds(l1top.removeFromLeft(44));
    lfo1ShapeBox_  .setBounds(l1top.withTrimmedRight(4));
    const int l1kW = l1c.getWidth() / 3;
    for (auto* k : { &lfo1RateKnob_, &lfo1DepthKnob_, &lfo1PhaseKnob_ })
        k->setBounds(l1c.removeFromLeft(l1kW));

    row3.removeFromLeft(pad);

    // LFO 2
    auto lfo2Bounds = row3.removeFromLeft(lfoW);
    lfo2Section_.setBounds(lfo2Bounds);
    auto l2c = lfo2Section_.getContentArea().translated(lfo2Bounds.getX(), lfo2Bounds.getY());
    auto l2top = l2c.removeFromTop(34);
    lfo2ShapeLabel_.setBounds(l2top.removeFromLeft(44));
    lfo2ShapeBox_  .setBounds(l2top.withTrimmedRight(4));
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

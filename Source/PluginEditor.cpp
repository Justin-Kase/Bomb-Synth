#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace BCol {
    const juce::Colour bg      { 0xFF0A0A14 };
    const juce::Colour panel   { 0xFF13131F };
    const juce::Colour accent  { 0xFFFF3B6F };   // hot pink/red — bomb red
    const juce::Colour accent2 { 0xFF4FC3F7 };   // sky blue
    const juce::Colour text    { 0xFFE0E0E0 };
    const juce::Colour textDim { 0xFF546E7A };
    const juce::Colour knobArc { 0xFFFF3B6F };
    const juce::Colour knobBg  { 0xFF1E1E30 };
}

BombSynthLookAndFeel::BombSynthLookAndFeel() {
    setColour(juce::Slider::thumbColourId,            BCol::accent);
    setColour(juce::Slider::rotarySliderFillColourId,  BCol::knobArc);
    setColour(juce::Slider::textBoxTextColourId,       BCol::text);
    setColour(juce::Slider::textBoxBackgroundColourId, BCol::panel);
    setColour(juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId,               BCol::text);
}

void BombSynthLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                             int x, int y, int w, int h,
                                             float sliderPos, float startAngle,
                                             float endAngle, juce::Slider&) {
    const float cx = x + w * 0.5f, cy = y + h * 0.5f;
    const float r  = std::min(w, h) * 0.36f;
    const float thick = r * 0.20f;
    const float valAngle = startAngle + sliderPos * (endAngle - startAngle);

    juce::Path bg;
    bg.addCentredArc(cx, cy, r, r, 0.f, startAngle, endAngle, true);
    g.setColour(BCol::knobBg);
    g.strokePath(bg, juce::PathStrokeType(thick, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));

    juce::Path arc;
    arc.addCentredArc(cx, cy, r, r, 0.f, startAngle, valAngle, true);
    g.setColour(BCol::knobArc);
    g.strokePath(arc, juce::PathStrokeType(thick, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

    // Dot
    g.setColour(BCol::accent.brighter(0.3f));
    g.fillEllipse(cx - 3.f, cy - 3.f, 6.f, 6.f);

    // Pointer
    const float px = cx + (r - thick * 0.5f) * std::sin(valAngle);
    const float py = cy - (r - thick * 0.5f) * std::cos(valAngle);
    g.setColour(juce::Colours::white);
    g.drawLine(cx, cy, px, py, 1.8f);
}

// ─── SynthKnob ───────────────────────────────────────────────────────────────
SynthKnob::SynthKnob(const juce::String& name) {
    slider.setSliderStyle(juce::Slider::Rotary);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
    addAndMakeVisible(slider);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(10.5f, juce::Font::bold));
    addAndMakeVisible(label);
}
void SynthKnob::resized() {
    auto b = getLocalBounds();
    label .setBounds(b.removeFromBottom(14));
    slider.setBounds(b);
}

// ─── Main Editor ─────────────────────────────────────────────────────────────
BombSynthAudioProcessorEditor::BombSynthAudioProcessorEditor(BombSynthAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), proc_(p)
{
    setLookAndFeel(&laf_);
    setSize(740, 340);
    setResizable(true, true);

    // Load logo
    int logoSize = 0;
    auto* logoData = BinaryData::getNamedResource("logo_png", logoSize);
    if (logoData && logoSize > 0)
        logo_ = juce::ImageFileFormat::loadFrom(logoData, logoSize);

    auto& params = proc_.parameters();

    attackAtt_   = std::make_unique<SliderAtt>(params, "amp_attack",    attackKnob_.slider);
    decayAtt_    = std::make_unique<SliderAtt>(params, "amp_decay",     decayKnob_.slider);
    sustainAtt_  = std::make_unique<SliderAtt>(params, "amp_sustain",   sustainKnob_.slider);
    releaseAtt_  = std::make_unique<SliderAtt>(params, "amp_release",   releaseKnob_.slider);
    cutoffAtt_   = std::make_unique<SliderAtt>(params, "filter_cutoff", cutoffKnob_.slider);
    resAtt_      = std::make_unique<SliderAtt>(params, "filter_res",    resKnob_.slider);
    lfo1RateAtt_ = std::make_unique<SliderAtt>(params, "lfo1_rate",     lfo1RateKnob_.slider);
    lfo1DepthAtt_= std::make_unique<SliderAtt>(params, "lfo1_depth",    lfo1DepthKnob_.slider);
    masterAtt_   = std::make_unique<SliderAtt>(params, "master_gain",   masterKnob_.slider);

    for (auto* k : { &attackKnob_, &decayKnob_, &sustainKnob_, &releaseKnob_,
                     &cutoffKnob_, &resKnob_, &lfo1RateKnob_, &lfo1DepthKnob_, &masterKnob_ })
        addAndMakeVisible(k);
}

BombSynthAudioProcessorEditor::~BombSynthAudioProcessorEditor() {
    setLookAndFeel(nullptr);
}

void BombSynthAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(BCol::bg);

    // Header bar
    const int hH = 46;
    g.setColour(BCol::panel);
    g.fillRect(0, 0, getWidth(), hH);

    // Logo
    if (logo_.isValid()) {
        const int ls = 30, lx = getWidth() - ls - 14, ly = (hH - ls) / 2;
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.fillEllipse((float)lx, (float)ly, (float)ls, (float)ls);
        g.drawImage(logo_, lx, ly, ls, ls, 0, 0, logo_.getWidth(), logo_.getHeight());
    }

    // Title
    g.setColour(BCol::accent);
    g.setFont(juce::Font(20.f, juce::Font::bold));
    g.drawText("BOMB SYNTH", 14, 0, 300, hH, juce::Justification::centredLeft);
    g.setColour(BCol::textDim);
    g.setFont(juce::Font(10.f));
    g.drawText("v0.1.0", 14, 0, getWidth() - 60, hH, juce::Justification::centredRight);

    // Section labels
    g.setFont(juce::Font(10.f, juce::Font::bold));
    g.setColour(BCol::textDim);
    g.drawText("ENVELOPE",  12,  hH + 4, 280, 14, juce::Justification::left);
    g.drawText("FILTER",   300,  hH + 4, 200, 14, juce::Justification::left);
    g.drawText("LFO 1",    508,  hH + 4, 160, 14, juce::Justification::left);
    g.drawText("MASTER",   636,  hH + 4, 100, 14, juce::Justification::left);

    // Section dividers
    g.setColour(BCol::accent.withAlpha(0.3f));
    for (int x : { 294, 500, 630 })
        g.drawVerticalLine(x, (float)hH, (float)getHeight());
}

void BombSynthAudioProcessorEditor::resized() {
    const int hH = 46 + 18;   // header + label row
    auto area = getLocalBounds().withTrimmedTop(hH).reduced(8, 4);
    const int kH = area.getHeight();
    const int kW = 72;

    // Envelope
    attackKnob_ .setBounds(area.removeFromLeft(kW).withHeight(kH));
    decayKnob_  .setBounds(area.removeFromLeft(kW).withHeight(kH));
    sustainKnob_.setBounds(area.removeFromLeft(kW).withHeight(kH));
    releaseKnob_.setBounds(area.removeFromLeft(kW).withHeight(kH));

    area.removeFromLeft(14);  // gap at section divider

    // Filter
    cutoffKnob_.setBounds(area.removeFromLeft(kW).withHeight(kH));
    resKnob_   .setBounds(area.removeFromLeft(kW).withHeight(kH));

    area.removeFromLeft(14);

    // LFO 1
    lfo1RateKnob_ .setBounds(area.removeFromLeft(kW).withHeight(kH));
    lfo1DepthKnob_.setBounds(area.removeFromLeft(kW).withHeight(kH));

    area.removeFromLeft(14);

    // Master
    masterKnob_.setBounds(area.removeFromLeft(kW).withHeight(kH));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

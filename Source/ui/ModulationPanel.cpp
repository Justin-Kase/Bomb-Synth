#include "ModulationPanel.h"

static const char* kSrcNames[] = { "Off", "LFO 1", "LFO 2", "Velocity", "ModWheel" };
static const char* kDstNames[] = {
    "Off",
    // Filter
    "Cutoff", "Resonance", "Drive",
    // Global
    "Pitch", "Amp",
    // Morph
    "Osc1 Morph", "Osc2 Morph", "Osc3 Morph",
    // Tune
    "Osc1 Tune", "Osc2 Tune", "Osc3 Tune",
    // Fine
    "Osc1 Fine", "Osc2 Fine", "Osc3 Fine",
    // Level
    "Osc1 Level", "Osc2 Level", "Osc3 Level",
    // FM
    "Osc1 FM", "Osc2 FM", "Osc3 FM",
    // Detune
    "Osc1 Detune", "Osc2 Detune", "Osc3 Detune",
    // LFO
    "LFO2 Rate",
    // Amp envelope
    "Amp Attack", "Amp Decay", "Amp Sustain", "Amp Release",
    // Filter envelope
    "Filter Env Amt", "FEnv Attack", "FEnv Decay", "FEnv Sustain", "FEnv Release",
    // LFOs
    "LFO1 Rate", "LFO1 Depth", "LFO2 Depth",
    // Master
    "Master Vol"
};
static constexpr int kNumDstNames = 38;

ModulationPanel::ModulationPanel(juce::AudioProcessorValueTreeState& params)
    : params_(params)
{
    for (int i = 0; i < kNumSlots; ++i) {
        auto& row = slots_[i];
        const juce::Colour col { kSlotColours[i] };
        const juce::String s(i);

        // Source combo
        for (int k = 0; k < 5; ++k) row.srcBox.addItem(kSrcNames[k], k + 1);
        row.srcBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1A1A2E));
        row.srcBox.setColour(juce::ComboBox::textColourId,       juce::Colour(0xFFE0E0E0));
        row.srcBox.setColour(juce::ComboBox::outlineColourId,    col.withAlpha(0.4f));
        row.srcBox.setColour(juce::ComboBox::arrowColourId,      col);
        row.srcAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            params_, "mod" + s + "_src", row.srcBox);
        addAndMakeVisible(row.srcBox);

        // Destination combo
        for (int k = 0; k < kNumDstNames; ++k) row.dstBox.addItem(kDstNames[k], k + 1);
        row.dstBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1A1A2E));
        row.dstBox.setColour(juce::ComboBox::textColourId,       juce::Colour(0xFFE0E0E0));
        row.dstBox.setColour(juce::ComboBox::outlineColourId,    col.withAlpha(0.4f));
        row.dstBox.setColour(juce::ComboBox::arrowColourId,      col);
        row.dstAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            params_, "mod" + s + "_dst", row.dstBox);
        addAndMakeVisible(row.dstBox);

        // Amount slider (bipolar rotary)
        row.amtSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        row.amtSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 16);
        row.amtSlider.setRange(-1.0, 1.0);
        row.amtSlider.setColour(juce::Slider::thumbColourId,           col);
        row.amtSlider.setColour(juce::Slider::trackColourId,           col.withAlpha(0.3f));
        row.amtSlider.setColour(juce::Slider::backgroundColourId,      juce::Colour(0xFF111120));
        row.amtSlider.setColour(juce::Slider::textBoxTextColourId,     juce::Colour(0xFFE0E0E0));
        row.amtSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF1A1A2E));
        row.amtSlider.setColour(juce::Slider::textBoxOutlineColourId,  juce::Colours::transparentBlack);
        row.amtAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            params_, "mod" + s + "_amt", row.amtSlider);
        addAndMakeVisible(row.amtSlider);
    }
}

ModulationPanel::~ModulationPanel() {}

void ModulationPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xFF0A0A14));

    // Header
    g.setColour(juce::Colour(0xFF1A1A2E));
    g.fillRect(0, 0, getWidth(), 30);
    g.setColour(juce::Colour(0xFF4FC3F7).withAlpha(0.6f));
    g.drawHorizontalLine(30, 0.f, (float)getWidth());

    g.setFont(juce::Font(11.f, juce::Font::bold));
    g.setColour(juce::Colour(0xFF4FC3F7));
    g.drawText("MODULATION MATRIX", 14, 0, 200, 30, juce::Justification::centredLeft);

    // Column headers
    g.setFont(juce::Font(8.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xFF607080));
    const int hY = 32;
    g.drawText("SOURCE",   14,              hY, 100, 12, juce::Justification::centredLeft);
    g.drawText("TARGET",   124,             hY, 110, 12, juce::Justification::centredLeft);
    g.drawText("AMOUNT",   244,             hY, 200, 12, juce::Justification::centredLeft);

    // Row backgrounds
    const int slotH = 34;
    for (int i = 0; i < kNumSlots; ++i) {
        const int rowY = 46 + i * slotH;
        const juce::Colour col { kSlotColours[i] };

        g.setColour(juce::Colour(0xFF111120));
        g.fillRoundedRectangle(4.f, (float)rowY, getWidth() - 8.f, (float)(slotH - 2), 4.f);

        // Lane number pill
        g.setColour(col.withAlpha(0.15f));
        g.fillRoundedRectangle(6.f, rowY + 7.f, 16.f, slotH - 16.f, 3.f);
        g.setColour(col.withAlpha(0.9f));
        g.setFont(juce::Font(8.f, juce::Font::bold));
        g.drawText(juce::String(i + 1), 6, rowY + 7, 16, slotH - 16,
                   juce::Justification::centred);

        // Bottom divider
        g.setColour(juce::Colour(0xFF252540));
        g.drawHorizontalLine(rowY + slotH - 2, 4.f, getWidth() - 4.f);
    }
}

void ModulationPanel::resized() {
    const int slotH  = 34;
    const int startY = 46;
    const int pad    = 4;

    for (int i = 0; i < kNumSlots; ++i) {
        auto& row  = slots_[i];
        int   rowY = startY + i * slotH + 5;
        int   rowH = slotH - 12;

        // Lane pill is 24px wide, then 4px gap
        row.srcBox.setBounds(26 + pad,  rowY, 92,  rowH);
        row.dstBox.setBounds(122 + pad, rowY, 110, rowH);
        row.amtSlider.setBounds(236 + pad, rowY, getWidth() - 244, rowH);
    }
}

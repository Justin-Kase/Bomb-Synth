#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <memory>

// ─── ModulationPanel ──────────────────────────────────────────────────────────
//  Shows 8 mod-routing slots with SOURCE / TARGET / AMOUNT controls.
class ModulationPanel : public juce::Component {
public:
    explicit ModulationPanel(juce::AudioProcessorValueTreeState& params);
    ~ModulationPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int kNumSlots = 8;

    // Lane accent colours
    static constexpr uint32_t kSlotColours[kNumSlots] = {
        0xFF4FC3F7, 0xFF00E676, 0xFFFFD740, 0xFFFF80AB,
        0xFFCE93D8, 0xFF80DEEA, 0xFFFF8A65, 0xFFB0BEC5,
    };

    struct SlotRow {
        juce::ComboBox    srcBox;
        juce::ComboBox    dstBox;
        juce::Slider      amtSlider;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> srcAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> dstAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   amtAtt;
    };

    std::array<SlotRow, kNumSlots> slots_;
    juce::AudioProcessorValueTreeState& params_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationPanel)
};

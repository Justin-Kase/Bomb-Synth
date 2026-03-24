#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../engine/oscillators/WavetableBank.h"
#include <functional>

// ─── WavetableDisplay ──────────────────────────────────────────────────────────
//
//  Shows the current wavetable frame as an oscilloscope waveform with a glow
//  effect.  Left/right arrow regions navigate between banks.  The morph position
//  is visualised by blending two frames (same as the audio engine does).
//
class WavetableDisplay : public juce::Component {
public:
    /** Called when the user clicks the navigation arrows. */
    std::function<void(int newBankIdx)> onBankChanged;

    /** Called when the user clicks the load-wavetable button (↑ icon). */
    std::function<void()> onLoadWavetable;

    WavetableDisplay();

    void setBankIndex(int idx);
    void setMorphPos (float pos01);   // [0, 1]

    int   getBankIndex() const { return bankIdx_; }
    float getMorphPos()  const { return morphPos_; }

    // juce::Component overrides
    void paint        (juce::Graphics&)                     override;
    void mouseMove    (const juce::MouseEvent&)             override;
    void mouseDown    (const juce::MouseEvent&)             override;
    void mouseExit    (const juce::MouseEvent&)             override;
    void mouseDrag    (const juce::MouseEvent&)             override {}


private:
    int   bankIdx_  = 0;
    float morphPos_ = 0.f;
    bool  hoverL_    = false;
    bool  hoverR_    = false;
    bool  hoverLoad_ = false;

    juce::Path buildWaveformPath(const juce::Rectangle<float>& area) const;

    static constexpr int kNavW    = 18;  // px width of each arrow region
    static constexpr int kBarH    = 20;  // px height of bottom nav bar
    static constexpr int kDotRow  =  6;  // px height of frame-dot row
    static constexpr int kHdrH    = 14;  // px height of top label row
    static constexpr int kLoadW   = 18;  // px width of the load button (top-right of header)
};

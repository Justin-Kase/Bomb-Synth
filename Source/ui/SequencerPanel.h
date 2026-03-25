#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../engine/Sequencer.h"
#include <functional>

// ─── SequencerPanel ──────────────────────────────────────────────────────────
class SequencerPanel : public juce::Component,
                       private juce::Timer {
public:
    // Called when user edits any step/lane data — host should save state
    std::function<void()> onStateChanged;

    explicit SequencerPanel(Sequencer& seq);
    ~SequencerPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

    // Called by editor to trigger visual update of playhead
    void timerCallback() override;

    // ── Note names ──────────────────────────────────────────────────────────
    static juce::String noteName(int midi);

private:
    Sequencer& seq_;

    // Lane colors (matching bank palette style)
    static constexpr uint32_t kLaneColours[4] = {
        0xFF4FC3F7,   // sky blue
        0xFF00E676,   // green
        0xFFFFD740,   // amber
        0xFFFF80AB,   // pink
    };

    // UI state
    int  selectedLane_ = 0;
    int  selectedStep_ = 0;
    bool dragging_     = false;
    float dragStartY_  = 0.f;
    float dragOrigVel_ = 0.8f;

    // Layout constants
    static constexpr int kHeaderH   = 44;   // global controls bar
    static constexpr int kLaneHdrW  = 200;  // lane label column width
    static constexpr int kDetailH   = 56;   // step detail panel height
    static constexpr int kStepMinW  = 16;   // minimum step cell width

    // Cached step rects (rebuilt in resized)
    struct StepRect { juce::Rectangle<int> r; int lane, step; };
    std::vector<StepRect> stepRects_;

    // Control widget rects (built in resized)
    juce::Rectangle<int> playBtnR_, stopBtnR_, bpmLabelR_, syncBtnR_;
    juce::Rectangle<int> rootBtnR_, scaleBtnR_, lanesDecR_, lanesIncR_;
    juce::Rectangle<int> arpBtnR_, arpModeR_;  // kept for global toolbar remnants
    std::array<juce::Rectangle<int>, Sequencer::kMaxLanes> stepsDecR_, stepsIncR_,
        octDecR_, octIncR_, swingR_, laneActiveR_,
        laneArpBtnR_, laneArpModeR_, laneArpRateR_, laneArpOctR_, laneRndBtnR_;

    juce::Rectangle<int> detailR_;

    // Step detail controls
    juce::Rectangle<int> detNoteBtnR_[2], detVelBtnR_[2], detProbBtnR_[2], detUTimeBtnR_[2];

    void rebuildStepRects(juce::Rectangle<int> laneArea, int laneIdx);
    void drawGlobalControls(juce::Graphics&);
    void drawLane(juce::Graphics&, int laneIdx, juce::Rectangle<int> bounds);
    void drawStepDetail(juce::Graphics&);

    const StepRect* hitTestStep(juce::Point<int> pt) const;
    void handleGlobalClick(juce::Point<int> pt);
    void handleLaneClick(juce::Point<int> pt, int lane);

    static juce::Colour laneColour(int l) { return juce::Colour(kLaneColours[l % 4]); }

    // combobox-style cycle helpers
    static const char* kNoteNames[];
};

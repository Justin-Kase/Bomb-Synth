#include "SequencerPanel.h"

const char* SequencerPanel::kNoteNames[] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};

juce::String SequencerPanel::noteName(int midi) {
    return juce::String(kNoteNames[midi % 12]) + juce::String(midi / 12 - 1);
}

SequencerPanel::SequencerPanel(Sequencer& seq) : seq_(seq) {
    setOpaque(false);
    startTimerHz(24);
}
SequencerPanel::~SequencerPanel() { stopTimer(); }

// ─── Layout ───────────────────────────────────────────────────────────────────
void SequencerPanel::resized() {
    stepRects_.clear();
    auto bounds = getLocalBounds();

    // Global controls strip
    auto hdr = bounds.removeFromTop(kHeaderH);
    int gap = 4;
    int x = hdr.getX() + gap;
    int cy = hdr.getCentreY();
    playBtnR_ = { x, cy - 14, 34, 28 };  x += 34 + gap;
    stopBtnR_ = { x, cy - 14, 34, 28 };  x += 34 + gap * 3;
    // BPM with dec/inc buttons
    bpmDecR_  = { x, cy - 14, 24, 28 };  x += 24;
    bpmLabelR_= { x, cy - 14, 64, 28 };  x += 64;
    bpmIncR_  = { x, cy - 14, 24, 28 };  x += 24 + gap;
    syncBtnR_ = { x, cy - 14, 68, 28 };  x += 68 + gap * 3;
    rootBtnR_ = { x, cy - 14, 44, 28 };  x += 44 + gap;
    scaleBtnR_= { x, cy - 14, 100, 28 }; x += 100 + gap * 3;
    // Lanes count
    lanesDecR_= { x,       cy - 10, 20, 20 };
    lanesIncR_= { x + 50,  cy - 10, 20, 20 };
    x += 80 + gap * 3;
    arpBtnR_  = { x, cy - 14, 50, 28 }; x += 54 + gap;
    arpModeR_ = { x, cy - 14, 90, 28 };

    // Detail strip (bottom)
    detailR_ = bounds.removeFromBottom(kDetailH);

    // Distribute remaining height among lanes
    const int nL = seq_.numActiveLanes;
    for (int l = 0; l < nL; ++l) {
        int lH = bounds.getHeight() / (nL - l);
        auto lBounds = bounds.removeFromTop(lH);

        // Lane header — 5-row layout
        auto lHdr = lBounds.removeFromLeft(kLaneHdrW).reduced(3, 2);
        const int rowH = lHdr.getHeight() / 5;
        const int px   = lHdr.getX();

        // Row 0: Lane label + [RND] button
        auto row0 = lHdr.removeFromTop(rowH);
        laneActiveR_[l]  = { px, row0.getCentreY() - 8, 14, 16 };
        laneRndBtnR_[l]  = { lHdr.getRight() - 38, row0.getY(), 38, rowH - 2 };

        // Row 1: Steps spinner
        auto row1 = lHdr.removeFromTop(rowH);
        stepsDecR_[l] = { px + 18, row1.getCentreY() - 9, 18, 18 };
        stepsIncR_[l] = { px + 60, row1.getCentreY() - 9, 18, 18 };

        // Row 2: Octave spinner
        auto row2 = lHdr.removeFromTop(rowH);
        octDecR_[l] = { px + 18, row2.getCentreY() - 9, 18, 18 };
        octIncR_[l] = { px + 60, row2.getCentreY() - 9, 18, 18 };

        // Row 3: Swing spinner
        auto row3 = lHdr.removeFromTop(rowH);
        swingR_[l * 2]     = { px + 18, row3.getCentreY() - 9, 18, 18 };  // dec
        swingR_[l * 2 + 1] = { px + 60, row3.getCentreY() - 9, 18, 18 };  // inc

        // Row 4: Per-lane ARP controls: [ARP] [mode] [rate] [oct]
        auto row4 = lHdr;
        const int aw = (row4.getWidth()) / 4 - 2;
        laneArpBtnR_ [l] = { row4.getX(),              row4.getY(), aw,     row4.getHeight() - 2 };
        laneArpModeR_[l] = { row4.getX() + aw + 2,    row4.getY(), aw + 8, row4.getHeight() - 2 };
        laneArpRateR_[l] = { row4.getX() + aw*2 + 12, row4.getY(), aw,     row4.getHeight() - 2 };
        laneArpOctR_ [l] = { row4.getX() + aw*3 + 14, row4.getY(), aw,     row4.getHeight() - 2 };

        // Step grid
        rebuildStepRects(lBounds.reduced(2), l);
    }

    // Detail controls (note, vel, prob, utime, glen) — 5 fields
    auto det = detailR_.reduced(8, 6);
    int dw = 96, dgap = 8;
    auto makeSpinner = [&](juce::Rectangle<int>* dec, juce::Rectangle<int>* inc, int startX) {
        *dec = { startX, det.getCentreY() - 11, 22, 22 };
        *inc = { startX + dw - 22, det.getCentreY() - 11, 22, 22 };
    };
    makeSpinner(&detNoteBtnR_[0],  &detNoteBtnR_[1],  det.getX());
    makeSpinner(&detVelBtnR_[0],   &detVelBtnR_[1],   det.getX() + dw + dgap);
    makeSpinner(&detProbBtnR_[0],  &detProbBtnR_[1],  det.getX() + (dw + dgap) * 2);
    makeSpinner(&detUTimeBtnR_[0], &detUTimeBtnR_[1], det.getX() + (dw + dgap) * 3);
    makeSpinner(&detGlenBtnR_[0],  &detGlenBtnR_[1],  det.getX() + (dw + dgap) * 4);
}

void SequencerPanel::rebuildStepRects(juce::Rectangle<int> area, int laneIdx) {
    const int n    = seq_.lanes[laneIdx].numSteps;
    const int cellW = std::max(kStepMinW, area.getWidth() / n);
    const int cellH = area.getHeight() - 4;
    for (int s = 0; s < n; ++s) {
        int x = area.getX() + s * cellW;
        stepRects_.push_back({{ x, area.getY() + 2, cellW - 2, cellH }, laneIdx, s });
    }
}

// ─── Paint ────────────────────────────────────────────────────────────────────
void SequencerPanel::paint(juce::Graphics& g) {
    // Background
    g.setColour(juce::Colour(0xFF0A0A14));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.f);

    drawGlobalControls(g);

    const int nL = seq_.numActiveLanes;
    auto bounds = getLocalBounds().withTrimmedTop(kHeaderH).withTrimmedBottom(kDetailH);
    for (int l = 0; l < nL; ++l) {
        int lH = bounds.getHeight() / (nL - l);
        drawLane(g, l, bounds.removeFromTop(lH));
    }

    drawStepDetail(g);
}

void SequencerPanel::drawGlobalControls(juce::Graphics& g) {
    // Header gradient
    auto hdrR = getLocalBounds().removeFromTop(kHeaderH).toFloat().reduced(2);
    juce::ColourGradient grad(juce::Colour(0xFF1A1A38), hdrR.getX(), hdrR.getY(),
                              juce::Colour(0xFF12121E), hdrR.getX(), hdrR.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(hdrR, 6.f);

    auto btn = [&](juce::Rectangle<int> r, const juce::String& label,
                   bool active, juce::Colour c = juce::Colour(0xFF4FC3F7)) {
        g.setColour(active ? c.withAlpha(0.3f) : juce::Colour(0xFF1E2030));
        g.fillRoundedRectangle(r.toFloat(), 4.f);
        g.setColour(active ? c : juce::Colour(0xFF607080));
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 4.f, 1.f);
        g.setFont(juce::Font(10.f, juce::Font::bold));
        g.setColour(active ? c : juce::Colour(0xFF8090A0));
        g.drawText(label, r, juce::Justification::centred);
    };

    btn(playBtnR_, seq_.playing ? "||" : "▶", seq_.playing, juce::Colour(0xFF00E676));
    btn(stopBtnR_, "■",  false, juce::Colour(0xFFFF3B6F));
    btn(syncBtnR_, seq_.syncDAW ? "DAW SYNC" : "FREE", seq_.syncDAW, juce::Colour(0xFFFFD740));

    // BPM display with dec/inc
    g.setColour(juce::Colour(0xFF1E2030));
    g.fillRoundedRectangle(bpmDecR_.toFloat(), 4.f);
    g.fillRoundedRectangle(bpmLabelR_.toFloat(), 4.f);
    g.fillRoundedRectangle(bpmIncR_.toFloat(), 4.f);
    g.setColour(juce::Colour(0xFF4FC3F7).withAlpha(0.6f));
    g.setFont(juce::Font(10.f, juce::Font::bold));
    g.drawText("<", bpmDecR_, juce::Justification::centred);
    g.drawText(">", bpmIncR_, juce::Justification::centred);
    g.setColour(juce::Colour(0xFF607080));
    g.drawRoundedRectangle(bpmLabelR_.toFloat().reduced(0.5f), 4.f, 1.f);
    g.setFont(juce::Font(11.f, juce::Font::bold));
    g.setColour(juce::Colour(0xFFE0E0E0));
    g.drawText(juce::String(seq_.bpm, 1), bpmLabelR_, juce::Justification::centred);

    // Root note
    btn(rootBtnR_, noteName(seq_.rootNote), false, juce::Colour(0xFFBA68C8));

    // Scale
    btn(scaleBtnR_, SeqScale::kNames[juce::jlimit(0, SeqScale::kCount - 1, seq_.scaleIdx)],
        seq_.scaleIdx > 0, juce::Colour(0xFFBA68C8));

    // Lanes
    g.setFont(juce::Font(9.f));
    g.setColour(juce::Colour(0xFF607080));
    auto lanesArea = juce::Rectangle<int>(lanesDecR_.getX(), lanesDecR_.getY(),
                                          lanesIncR_.getRight() - lanesDecR_.getX(), 20);
    g.drawText("LANES", lanesArea.translated(0, -12), juce::Justification::centred);
    btn(lanesDecR_, "<", false);
    g.setFont(juce::Font(11.f, juce::Font::bold));
    g.setColour(juce::Colour(0xFFE0E0E0));
    g.drawText(juce::String(seq_.numActiveLanes),
               juce::Rectangle<int>(lanesDecR_.getRight(), lanesDecR_.getY(), 16, 20),
               juce::Justification::centred);
    btn(lanesIncR_, ">", false);

    // (ARP controls moved to per-lane header)
}

void SequencerPanel::drawLane(juce::Graphics& g, int laneIdx, juce::Rectangle<int> bounds) {
    auto& lane = seq_.lanes[laneIdx];
    const juce::Colour lc = laneColour(laneIdx);

    // Lane header bg
    auto lHdr = bounds.removeFromLeft(kLaneHdrW);
    g.setColour(juce::Colour(0xFF111120));
    g.fillRoundedRectangle(lHdr.toFloat().reduced(1), 4.f);
    g.setColour(lc.withAlpha(0.3f));
    g.drawRoundedRectangle(lHdr.toFloat().reduced(1.5f), 4.f, 1.f);

    // ── Lane header contents ──────────────────────────────────────────────────
    auto smallBtn = [&](juce::Rectangle<int> r, const juce::String& label,
                        bool active, juce::Colour c) {
        g.setColour(active ? c.withAlpha(0.28f) : juce::Colour(0xFF171727));
        g.fillRoundedRectangle(r.toFloat(), 3.f);
        g.setColour(active ? c : juce::Colour(0xFF405060));
        g.drawRoundedRectangle(r.toFloat().reduced(0.4f), 3.f, 0.8f);
        g.setFont(juce::Font(8.5f, juce::Font::bold));
        g.setColour(active ? c : juce::Colour(0xFF708090));
        g.drawText(label, r, juce::Justification::centred);
    };

    // Row 0: "LANE N" + active dot + [RND]
    g.setFont(juce::Font(9.f, juce::Font::bold));
    g.setColour(lc);
    g.drawText("LANE " + juce::String(laneIdx + 1),
               lHdr.getX() + 18, lHdr.getY() + 2, kLaneHdrW - 60, 16, juce::Justification::centredLeft);
    // Active dot
    auto& adr = laneActiveR_[laneIdx];
    g.setColour(lane.active ? lc : juce::Colour(0xFF334455));
    g.fillEllipse(adr.toFloat().reduced(3));

    // [RND] button — orange accent
    smallBtn(laneRndBtnR_[laneIdx], "RND", false, juce::Colour(0xFFFF9800));

    // Row 1: Steps spinner
    auto drawSpinner = [&](juce::Rectangle<int> dec, juce::Rectangle<int> inc,
                            const juce::String& val, const juce::String& lbl) {
        g.setColour(juce::Colour(0xFF1A1A2C));
        g.fillRoundedRectangle(dec.toFloat(), 3.f);
        g.fillRoundedRectangle(inc.toFloat(), 3.f);
        g.setColour(lc.withAlpha(0.6f));
        g.setFont(juce::Font(9.f, juce::Font::bold));
        g.drawText("<", dec, juce::Justification::centred);
        g.drawText(">", inc, juce::Justification::centred);
        g.setColour(juce::Colour(0xFFE0E0E0));
        g.setFont(juce::Font(10.f, juce::Font::bold));
        g.drawText(val, dec.getRight(), dec.getY(), inc.getX() - dec.getRight(), dec.getHeight(),
                   juce::Justification::centred);
        g.setFont(juce::Font(7.5f));
        g.setColour(juce::Colour(0xFF607080));
        g.drawText(lbl, dec.getX() - 2, dec.getY(), 18, dec.getHeight(), juce::Justification::centredRight);
    };

    drawSpinner(stepsDecR_[laneIdx], stepsIncR_[laneIdx], juce::String(lane.numSteps), "STP");
    drawSpinner(octDecR_[laneIdx], octIncR_[laneIdx],
                juce::String(lane.octave > 0 ? "+" : "") + juce::String(lane.octave), "OCT");

    // Row 3: Swing spinner
    int swingPct = (int)(lane.swing * 100);
    drawSpinner(swingR_[laneIdx * 2], swingR_[laneIdx * 2 + 1],
                juce::String(swingPct) + "%", "SWG");

    // Row 3: Per-lane ARP controls
    const juce::Colour arpCol(0xFFFF9800);
    smallBtn(laneArpBtnR_[laneIdx], "ARP", lane.arpEnabled, arpCol);

    // ARP mode button
    if (lane.arpEnabled) {
        smallBtn(laneArpModeR_[laneIdx], ArpMode::kNames[lane.arpMode % ArpMode::Count], true, arpCol);
        // ARP rate
        static const char* rateNames[] = { "1/4","1/8","1/16","1/32" };
        static const int   rateDivs[]  = { 4, 8, 16, 32 };
        int rateNameIdx = 2; // default 1/16
        for (int ri = 0; ri < 4; ++ri) if (rateDivs[ri] == lane.arpRateDiv) { rateNameIdx = ri; break; }
        smallBtn(laneArpRateR_[laneIdx], rateNames[rateNameIdx], true, arpCol);
        // ARP octaves
        smallBtn(laneArpOctR_[laneIdx], juce::String(lane.arpOctaves) + "oct", true, arpCol);
    } else {
        g.setColour(juce::Colour(0xFF243040));
        g.fillRoundedRectangle(laneArpModeR_[laneIdx].toFloat(), 3.f);
        g.setFont(juce::Font(8.f));
        g.setColour(juce::Colour(0xFF405060));
        g.drawText(ArpMode::kNames[lane.arpMode % ArpMode::Count], laneArpModeR_[laneIdx], juce::Justification::centred);
        g.fillRoundedRectangle(laneArpRateR_[laneIdx].toFloat(), 3.f);
        g.fillRoundedRectangle(laneArpOctR_[laneIdx].toFloat(), 3.f);
    }

    // Step grid bg
    g.setColour(juce::Colour(0xFF0E0E1C));
    g.fillRoundedRectangle(bounds.toFloat().reduced(1), 4.f);

    // Lane separator line at top of this lane's full width
    g.setColour(lc.withAlpha(0.15f));
    g.fillRect(juce::Rectangle<int>(0, bounds.getY(), getWidth(), 1));

    // Draw step cells for this lane
    for (const auto& sr : stepRects_) {
        if (sr.lane != laneIdx) continue;
        const auto& step = lane.steps[sr.step];
        const bool  isCurrent = (sr.step == lane.currentStep) && seq_.playing;
        const bool  isSelected = (selectedLane_ == laneIdx && selectedStep_ == sr.step);
        const bool  isEven = (sr.step % 2 == 0);

        auto r = sr.r.toFloat();

        // Beat marker background (every 4 steps)
        bool isBeatStart = (sr.step % 4 == 0);
        g.setColour(isBeatStart ? juce::Colour(0xFF141428) : (isEven ? juce::Colour(0xFF111128) : juce::Colour(0xFF0D0D20)));
        g.fillRoundedRectangle(r, 3.f);

        if (step.gate) {
            // Velocity fill with gradient (brighter at top) — guard against zero height
            float fillH = r.getHeight() * step.vel;
            if (fillH >= 1.f) {
                auto fillR  = r.withTop(r.getBottom() - fillH);
                float alpha = 0.25f + step.prob * 0.75f;
                float a = isCurrent ? alpha : alpha * 0.7f;
                juce::ColourGradient velGrad(lc.brighter(0.4f).withAlpha(a), fillR.getX(), fillR.getY(),
                                             lc.withAlpha(a * 0.6f), fillR.getX(), fillR.getBottom() + 1.f, false);
                g.setGradientFill(velGrad);
                g.fillRoundedRectangle(fillR, 3.f);
            }

            // Gate length bar at top (3px height)
            float gateBarW = r.getWidth() * juce::jlimit(0.f, 1.f, step.gateLen);
            g.setColour(lc.withAlpha(0.6f));
            g.fillRect(juce::Rectangle<float>(r.getX(), r.getY(), gateBarW, 3.f));

            // Note name overlay (only if cell >= 28px wide)
            if (sr.r.getWidth() >= 28) {
                g.setColour(juce::Colours::white.withAlpha(0.7f));
                g.setFont(juce::Font(7.5f));
                int displayNote = juce::jlimit(0, 127, step.note + lane.octave * 12);
                g.drawText(noteName(displayNote), r.reduced(1.f, 4.f), juce::Justification::centred, false);
            }

            // Playhead glow: bright top-edge line
            if (isCurrent) {
                g.setColour(lc.withAlpha(1.0f));
                g.fillRect(juce::Rectangle<float>(r.getX(), r.getY(), r.getWidth(), 2.f));
            }
        } else {
            // Step number (bottom-right) when gate is off
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.setFont(juce::Font(6.5f));
            g.drawText(juce::String(sr.step + 1), r.reduced(1.f, 1.f), juce::Justification::bottomRight, false);
        }

        // Microtiming indicator (small triangle at bottom)
        if (std::abs(step.utime) > 0.01f) {
            float cx  = r.getCentreX() + step.utime * r.getWidth();
            float by  = r.getBottom() - 4.f;
            g.setColour(juce::Colour(0xFFFFD740).withAlpha(0.8f));
            juce::Path tri;
            tri.addTriangle(cx - 3.f, by + 4.f, cx + 3.f, by + 4.f, cx, by);
            g.fillPath(tri);
        }

        // Probability dot (bottom centre, size = prob)
        if (step.prob < 0.99f) {
            float dotR = 2.5f * step.prob;
            g.setColour(juce::Colour(0xFFFF9800).withAlpha(0.9f));
            g.fillEllipse(r.getCentreX() - dotR, r.getBottom() - dotR * 2 - 2.f, dotR * 2, dotR * 2);
        }

        // Playhead highlight
        if (isCurrent) {
            g.setColour(lc.withAlpha(0.25f));
            g.fillRoundedRectangle(r, 3.f);
            g.setColour(lc.brighter(0.3f));
            g.drawRoundedRectangle(r.reduced(0.5f), 3.f, 1.5f);
        }

        // Selection border
        if (isSelected) {
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.drawRoundedRectangle(r.reduced(1.f), 3.f, 1.2f);
        } else {
            g.setColour(juce::Colour(0xFF252540));
            g.drawRoundedRectangle(r, 3.f, 0.5f);
        }
    }
}

void SequencerPanel::drawStepDetail(juce::Graphics& g) {
    auto r = detailR_.toFloat();
    g.setColour(juce::Colour(0xFF111120));
    g.fillRoundedRectangle(r, 6.f);
    g.setColour(juce::Colour(0xFF252540));
    g.drawRoundedRectangle(r.reduced(0.5f), 6.f, 1.f);

    if (selectedLane_ >= seq_.numActiveLanes) return;
    const auto& step = seq_.lanes[selectedLane_].steps[selectedStep_];
    const juce::Colour lc = laneColour(selectedLane_);

    // Title
    g.setFont(juce::Font(9.f, juce::Font::bold));
    g.setColour(lc.withAlpha(0.7f));
    g.drawText("L" + juce::String(selectedLane_ + 1) + " STEP " + juce::String(selectedStep_ + 1),
               detailR_.getX() + 8, detailR_.getY() + 4, 80, 12, juce::Justification::left);

    auto drawField = [&](const juce::String& label, const juce::String& val,
                         juce::Rectangle<int> dec, juce::Rectangle<int> inc) {
        juce::Rectangle<int> labelR { dec.getX(), dec.getY() - 14, inc.getRight() - dec.getX(), 12 };
        g.setFont(juce::Font(8.f));
        g.setColour(juce::Colour(0xFF607080));
        g.drawText(label, labelR, juce::Justification::centred);

        g.setColour(juce::Colour(0xFF1A1A2E));
        g.fillRoundedRectangle(dec.toFloat(), 3.f);
        g.fillRoundedRectangle(inc.toFloat(), 3.f);
        g.setColour(lc.withAlpha(0.6f));
        g.setFont(juce::Font(10.f, juce::Font::bold));
        g.drawText("<", dec, juce::Justification::centred);
        g.drawText(">", inc, juce::Justification::centred);

        g.setFont(juce::Font(11.f, juce::Font::bold));
        g.setColour(juce::Colour(0xFFE0E0E0));
        juce::Rectangle<int> valR { dec.getRight() + 2, dec.getY(), inc.getX() - dec.getRight() - 4, dec.getHeight() };
        g.drawText(val, valR, juce::Justification::centred);
    };

    drawField("NOTE",   noteName(juce::jlimit(0, 127, step.note + seq_.lanes[selectedLane_].octave * 12)),
              detNoteBtnR_[0],  detNoteBtnR_[1]);
    drawField("VEL",    juce::String((int)(step.vel  * 100)) + "%",  detVelBtnR_[0],   detVelBtnR_[1]);
    drawField("PROB",   juce::String((int)(step.prob * 100)) + "%",  detProbBtnR_[0],  detProbBtnR_[1]);
    drawField("\u03bcTIME",  juce::String((int)(step.utime * 100)) + "%", detUTimeBtnR_[0], detUTimeBtnR_[1]);
    drawField("GLEN",   juce::String((int)(step.gateLen * 100)) + "%", detGlenBtnR_[0],  detGlenBtnR_[1]);

    // Gate indicator
    g.setFont(juce::Font(9.f));
    g.setColour(step.gate ? lc : juce::Colour(0xFF607080));
    g.drawText(step.gate ? "GATE ON" : "GATE OFF",
               detailR_.getRight() - 80, detailR_.getCentreY() - 8, 72, 16,
               juce::Justification::centred);

    // Keyboard shortcut hint
    g.setFont(juce::Font(7.f));
    g.setColour(juce::Colour(0xFF405060));
    g.drawText("Shift+click note \u00b1octave  |  drag step = velocity",
               detailR_.getX() + 8, detailR_.getBottom() - 14, detailR_.getWidth() - 90, 12,
               juce::Justification::right);
}

// ─── Mouse ────────────────────────────────────────────────────────────────────
void SequencerPanel::timerCallback() { repaint(); }

const SequencerPanel::StepRect* SequencerPanel::hitTestStep(juce::Point<int> pt) const {
    for (const auto& sr : stepRects_)
        if (sr.r.contains(pt)) return &sr;
    return nullptr;
}

void SequencerPanel::mouseDown(const juce::MouseEvent& e) {
    auto pt = e.getPosition();

    // Check global controls
    if (pt.getY() < kHeaderH) { handleGlobalClick(pt, e.mods.isShiftDown()); return; }

    // Check detail controls
    if (detailR_.contains(pt)) {
        if (selectedLane_ >= seq_.numActiveLanes) return;
        auto& step = seq_.lanes[selectedLane_].steps[selectedStep_];
        float noteStep = e.mods.isShiftDown() ? 12.f : 1.f;
        if (detNoteBtnR_[0].contains(pt)) { step.note = juce::jlimit(0,127, step.note - (int)noteStep); if(onStateChanged) onStateChanged(); repaint(); return; }
        if (detNoteBtnR_[1].contains(pt)) { step.note = juce::jlimit(0,127, step.note + (int)noteStep); if(onStateChanged) onStateChanged(); repaint(); return; }
        if (detVelBtnR_[0].contains(pt))  { step.vel  = juce::jlimit(0.f,1.f, step.vel  - 0.05f); if(onStateChanged) onStateChanged(); repaint(); return; }
        if (detVelBtnR_[1].contains(pt))  { step.vel  = juce::jlimit(0.f,1.f, step.vel  + 0.05f); if(onStateChanged) onStateChanged(); repaint(); return; }
        if (detProbBtnR_[0].contains(pt)) { step.prob = juce::jlimit(0.f,1.f, step.prob - 0.1f);  if(onStateChanged) onStateChanged(); repaint(); return; }
        if (detProbBtnR_[1].contains(pt)) { step.prob = juce::jlimit(0.f,1.f, step.prob + 0.1f);  if(onStateChanged) onStateChanged(); repaint(); return; }
        if (detUTimeBtnR_[0].contains(pt)){ step.utime= juce::jlimit(-0.5f,0.5f, step.utime - 0.05f); if(onStateChanged) onStateChanged(); repaint(); return; }
        if (detUTimeBtnR_[1].contains(pt)){ step.utime= juce::jlimit(-0.5f,0.5f, step.utime + 0.05f); if(onStateChanged) onStateChanged(); repaint(); return; }
        if (detGlenBtnR_[0].contains(pt)) { step.gateLen = juce::jlimit(0.f,1.f, step.gateLen - 0.05f); if(onStateChanged) onStateChanged(); repaint(); return; }
        if (detGlenBtnR_[1].contains(pt)) { step.gateLen = juce::jlimit(0.f,1.f, step.gateLen + 0.05f); if(onStateChanged) onStateChanged(); repaint(); return; }
        return;
    }

    // Check lane header controls
    static const int rateDivs[] = { 4, 8, 16, 32 };
    for (int l = 0; l < seq_.numActiveLanes; ++l) {
        auto& lane = seq_.lanes[l];

        if (stepsDecR_[l].contains(pt)) { lane.numSteps = juce::jlimit(1, SequencerLane::kMaxSteps, lane.numSteps - 1); resized(); repaint(); if(onStateChanged) onStateChanged(); return; }
        if (stepsIncR_[l].contains(pt)) { lane.numSteps = juce::jlimit(1, SequencerLane::kMaxSteps, lane.numSteps + 1); resized(); repaint(); if(onStateChanged) onStateChanged(); return; }
        if (octDecR_[l].contains(pt))   { lane.octave   = juce::jlimit(-3, 3, lane.octave - 1); repaint(); if(onStateChanged) onStateChanged(); return; }
        if (octIncR_[l].contains(pt))   { lane.octave   = juce::jlimit(-3, 3, lane.octave + 1); repaint(); if(onStateChanged) onStateChanged(); return; }
        if (swingR_[l*2].contains(pt))  { lane.swing    = juce::jlimit(0.f, 1.f, lane.swing - 0.05f); repaint(); if(onStateChanged) onStateChanged(); return; }
        if (swingR_[l*2+1].contains(pt)){ lane.swing    = juce::jlimit(0.f, 1.f, lane.swing + 0.05f); repaint(); if(onStateChanged) onStateChanged(); return; }
        if (laneActiveR_[l].contains(pt)){ lane.active  = !lane.active; repaint(); if(onStateChanged) onStateChanged(); return; }

        // [RND] — randomize this lane's step pattern
        if (laneRndBtnR_[l].contains(pt)) {
            seq_.randomizeLane(l);
            repaint();
            if(onStateChanged) onStateChanged();
            return;
        }

        // [ARP] toggle
        if (laneArpBtnR_[l].contains(pt)) {
            lane.arpEnabled = !lane.arpEnabled;
            repaint();
            if(onStateChanged) onStateChanged();
            return;
        }
        // ARP mode cycle
        if (laneArpModeR_[l].contains(pt)) {
            lane.arpMode = (lane.arpMode + 1) % ArpMode::Count;
            repaint();
            if(onStateChanged) onStateChanged();
            return;
        }
        // ARP rate cycle: 4 → 8 → 16 → 32 → 4
        if (laneArpRateR_[l].contains(pt)) {
            int cur = 2;
            for (int ri = 0; ri < 4; ++ri) if (rateDivs[ri] == lane.arpRateDiv) { cur = ri; break; }
            lane.arpRateDiv = rateDivs[(cur + 1) % 4];
            repaint();
            if(onStateChanged) onStateChanged();
            return;
        }
        // ARP octaves cycle: 1–4
        if (laneArpOctR_[l].contains(pt)) {
            lane.arpOctaves = (lane.arpOctaves % 4) + 1;
            repaint();
            if(onStateChanged) onStateChanged();
            return;
        }
    }

    // Check step cells
    if (auto* sr = hitTestStep(pt)) {
        selectedLane_ = sr->lane;
        selectedStep_ = sr->step;
        auto& step = seq_.lanes[sr->lane].steps[sr->step];

        if (e.mods.isRightButtonDown()) {
            // Right-click: toggle gate
            step.gate = !step.gate;
        } else {
            // Left-click: select + begin drag for velocity
            dragging_    = true;
            dragStartY_  = (float)pt.y;
            dragOrigVel_ = step.vel;
            // Single click toggles gate
            if (!dragging_) step.gate = !step.gate;
        }
        if(onStateChanged) onStateChanged();
        repaint();
    }
}

void SequencerPanel::mouseDrag(const juce::MouseEvent& e) {
    if (!dragging_) return;
    auto pt = e.getPosition();
    if (auto* sr = hitTestStep(pt)) {
        if (sr->lane == selectedLane_ && sr->step == selectedStep_) {
            float dy = dragStartY_ - pt.y;
            float newVel = juce::jlimit(0.05f, 1.f, dragOrigVel_ + dy / 80.f);
            seq_.lanes[selectedLane_].steps[selectedStep_].vel = newVel;
            if(onStateChanged) onStateChanged();
            repaint();
        }
    }
}

void SequencerPanel::mouseUp(const juce::MouseEvent&) {
    if (dragging_) {
        dragging_ = false;
        // If barely moved, treat as toggle
        auto& step = seq_.lanes[selectedLane_].steps[selectedStep_];
        if (std::abs(step.vel - dragOrigVel_) < 0.02f)
            step.gate = !step.gate;
        if(onStateChanged) onStateChanged();
        repaint();
    }
}

void SequencerPanel::handleGlobalClick(juce::Point<int> pt, bool shiftDown) {
    if (playBtnR_.contains(pt)) {
        seq_.setPlaying(!seq_.playing);
    } else if (stopBtnR_.contains(pt)) {
        juce::MidiBuffer dummy;
        seq_.stop(dummy);
        for (auto& lane : seq_.lanes) lane.currentStep = 0;
    } else if (bpmDecR_.contains(pt)) {
        float step = shiftDown ? 10.f : 1.f;
        seq_.bpm = juce::jlimit(20.f, 300.f, seq_.bpm - step);
    } else if (bpmIncR_.contains(pt)) {
        float step = shiftDown ? 10.f : 1.f;
        seq_.bpm = juce::jlimit(20.f, 300.f, seq_.bpm + step);
    } else if (syncBtnR_.contains(pt)) {
        seq_.syncDAW = !seq_.syncDAW;
    } else if (rootBtnR_.contains(pt)) {
        seq_.rootNote = (seq_.rootNote + 1) % 12 + (seq_.rootNote / 12) * 12;
    } else if (scaleBtnR_.contains(pt)) {
        seq_.scaleIdx = (seq_.scaleIdx + 1) % SeqScale::kCount;
    } else if (lanesDecR_.contains(pt)) {
        seq_.numActiveLanes = juce::jmax(1, seq_.numActiveLanes - 1);
        resized();
    } else if (lanesIncR_.contains(pt)) {
        seq_.numActiveLanes = juce::jmin(Sequencer::kMaxLanes, seq_.numActiveLanes + 1);
        resized();
    }
    if(onStateChanged) onStateChanged();
    repaint();
}

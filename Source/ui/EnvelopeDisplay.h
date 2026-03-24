#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// ─── EnvelopeDisplay ─────────────────────────────────────────────────────────
//  Draws a real-time ADSR envelope shape.  Values are normalised [0,1] where
//  each time segment is mapped proportionally to its share of a fixed total.
class EnvelopeDisplay : public juce::Component {
public:
    juce::Colour colour { juce::Colour(0xFF00E676) };

    void setADSR(float a01, float d01, float s01, float r01) {
        a_ = a01; d_ = d01; s_ = s01; r_ = r01;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().toFloat().reduced(1.f);
        const float W = b.getWidth(), H = b.getHeight();
        const float bx = b.getX(),   by = b.getY();

        // Background
        g.setColour(juce::Colour(0xFF080810));
        g.fillRoundedRectangle(b, 4.f);
        g.setColour(colour.withAlpha(0.2f));
        g.drawRoundedRectangle(b.reduced(0.5f), 4.f, 1.f);

        // Build normalised x positions (proportional allocation of total time)
        // attack:  a_ * 0.35 of total
        // decay:   d_ * 0.25 of total
        // sustain: 0.15 constant hold
        // release: r_ * 0.35 of total
        // We also reserve 0.05 for the initial zero segment and 0.05 for end zero
        const float tA = a_ * 0.35f + 0.02f;
        const float tD = d_ * 0.25f + 0.01f;
        const float tH = 0.15f;
        const float tR = r_ * 0.35f + 0.02f;
        const float tTotal = tA + tD + tH + tR + 0.05f;

        const float x0  = bx;
        const float xA  = bx + (tA / tTotal) * W;
        const float xD  = xA + (tD / tTotal) * W;
        const float xH  = xD + (tH / tTotal) * W;
        const float xR  = xH + (tR / tTotal) * W;
        const float yBot = by + H - 1.f;
        const float yTop = by + 2.f;
        const float ySus = by + (1.f - juce::jlimit(0.f, 1.f, s_)) * (H - 4.f) + 2.f;

        // Filled shape
        juce::Path fill;
        fill.startNewSubPath(x0,  yBot);
        fill.lineTo         (xA,  yTop);
        fill.lineTo         (xD,  ySus);
        fill.lineTo         (xH,  ySus);
        fill.lineTo         (xR,  yBot);
        fill.lineTo         (x0,  yBot);
        fill.closeSubPath();

        juce::ColourGradient grad(colour.withAlpha(0.3f), 0, yTop,
                                   colour.withAlpha(0.05f), 0, yBot, false);
        g.setGradientFill(grad);
        g.fillPath(fill);

        // Outline stroke
        juce::Path line;
        line.startNewSubPath(x0,  yBot);
        line.lineTo         (xA,  yTop);
        line.lineTo         (xD,  ySus);
        line.lineTo         (xH,  ySus);
        line.lineTo         (xR,  yBot);

        g.setColour(colour.withAlpha(0.85f));
        g.strokePath(line, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        // Subtle phase labels
        g.setFont(juce::Font(7.f));
        g.setColour(colour.withAlpha(0.4f));
        g.drawText("A", (int)x0,  (int)yBot - 9, (int)(xA - x0), 8, juce::Justification::centred);
        g.drawText("D", (int)xA,  (int)yBot - 9, (int)(xD - xA), 8, juce::Justification::centred);
        g.drawText("S", (int)xD,  (int)yBot - 9, (int)(xH - xD), 8, juce::Justification::centred);
        g.drawText("R", (int)xH,  (int)yBot - 9, (int)(xR - xH), 8, juce::Justification::centred);
    }

private:
    float a_{ 0.02f }, d_{ 0.15f }, s_{ 0.75f }, r_{ 0.05f };
};

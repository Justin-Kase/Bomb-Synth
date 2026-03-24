#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

// ─── FilterDisplay ────────────────────────────────────────────────────────────
//  Animated frequency-response curve for the current filter type.
//  X: log 20Hz–20kHz   Y: 0..1 amplitude (normalised)
class FilterDisplay : public juce::Component {
public:
    // filterType: 0=LadderLP24 1=LadderLP12 2=LadderHP 3=SVFLP 4=Formant 5=Comb
    void setParams(int filterType, float cutoffHz, float resonance) {
        filterType_ = filterType;
        cutoffHz_   = cutoffHz;
        resonance_  = resonance;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto b  = getLocalBounds().toFloat().reduced(1.f);
        const float W = b.getWidth();
        const float H = b.getHeight();
        const float bx = b.getX(), by = b.getY();

        // Background
        g.setColour(juce::Colour(0xFF080810));
        g.fillRoundedRectangle(b, 4.f);
        g.setColour(juce::Colour(0xFF4FC3F7).withAlpha(0.2f));
        g.drawRoundedRectangle(b.reduced(0.5f), 4.f, 1.f);

        // Grid lines
        g.setColour(juce::Colour(0xFF4FC3F7).withAlpha(0.07f));
        for (float freq : { 100.f, 1000.f, 10000.f }) {
            float xf = freqToX(freq, W);
            g.drawVerticalLine((int)(bx + xf), by, by + H);
        }

        // Build curve
        constexpr int kPoints = 120;
        juce::Path fill, line;

        bool started = false;
        for (int i = 0; i < kPoints; ++i) {
            const float t    = (float)i / (kPoints - 1);
            const float freq = 20.f * std::pow(1000.f, t);      // 20Hz → 20kHz log
            const float mag  = juce::jlimit(0.f, 1.2f, magnitude(freq));

            const float px = bx + freqToX(freq, W);
            const float py = by + H - mag * H;

            if (!started) {
                fill.startNewSubPath(px, by + H);
                fill.lineTo(px, py);
                line.startNewSubPath(px, py);
                started = true;
            } else {
                fill.lineTo(px, py);
                line.lineTo(px, py);
            }
        }
        fill.lineTo(bx + W, by + H);
        fill.closeSubPath();

        const juce::Colour accent { 0xFF4FC3F7 };
        juce::ColourGradient grad(accent.withAlpha(0.25f), 0, by,
                                   accent.withAlpha(0.04f), 0, by + H, false);
        g.setGradientFill(grad);
        g.fillPath(fill);

        g.setColour(accent.withAlpha(0.9f));
        g.strokePath(line, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        // Cutoff marker
        float markerX = bx + freqToX(cutoffHz_, W);
        g.setColour(accent.withAlpha(0.35f));
        g.drawVerticalLine((int)markerX, by, by + H);

        // Label
        g.setFont(juce::Font(7.5f));
        g.setColour(accent.withAlpha(0.45f));
        static const char* kNames[] = { "LP24","LP12","HP","SVF LP","FORMANT","COMB" };
        if (filterType_ >= 0 && filterType_ <= 5)
            g.drawText(kNames[filterType_], (int)bx + 4, (int)by + 2, 50, 10,
                       juce::Justification::centredLeft);
    }

private:
    int   filterType_ = 0;
    float cutoffHz_   = 6000.f;
    float resonance_  = 0.f;

    static float freqToX(float freq, float width) {
        return std::log(freq / 20.f) / std::log(1000.f) * width;
    }

    float magnitude(float freq) const {
        const float fc = juce::jmax(20.f, cutoffHz_);
        const float r  = resonance_;
        const float ratio = freq / fc;

        switch (filterType_) {
            case 0: // Ladder LP24 — 4th-order Butterworth approx
                return 1.f / std::sqrt(1.f + std::pow(ratio, 8.f));

            case 1: // Ladder LP12 — 2nd-order
                return 1.f / std::sqrt(1.f + std::pow(ratio, 4.f));

            case 2: // Ladder HP24
                return 1.f / std::sqrt(1.f + std::pow(1.f / ratio, 8.f));

            case 3: { // SVF LP — 2nd-order with resonance
                const float Q   = 0.5f + r * 9.5f;
                const float den = std::sqrt(juce::square(1.f - ratio * ratio)
                                          + juce::square(ratio / Q));
                return (den > 1e-6f) ? (1.f / den) : 1.f;
            }

            case 4: { // Formant — three Gaussian peaks
                static const float kFreqs[5][3] = {
                    {730.f, 1090.f, 2440.f},
                    {270.f, 2290.f, 3010.f},
                    {390.f, 1990.f, 2550.f},
                    {570.f,  840.f, 2410.f},
                    {300.f,  870.f, 2240.f},
                };
                static const float kBWs[3] = { 80.f, 140.f, 200.f };
                int vowel = juce::jlimit(0, 4, (int)(r * 4.99f));
                float out = 0.f;
                for (int i = 0; i < 3; ++i) {
                    float d = (freq - kFreqs[vowel][i]) / kBWs[i];
                    out += std::exp(-0.5f * d * d);
                }
                return juce::jlimit(0.f, 1.2f, out * 0.7f);
            }

            case 5: { // Comb — standing-wave peaks
                // Simple cosine approximation of comb magnitude envelope
                const float feedback = juce::jlimit(0.f, 0.99f, r);
                const double w = juce::MathConstants<double>::twoPi * freq / fc;
                const float cosW = (float)std::cos(w);
                const float denom = std::sqrt(1.f + feedback * feedback
                                            - 2.f * feedback * cosW);
                return (denom > 1e-6f) ? 1.f / denom : 1.f;
            }

            default:
                return 1.f;
        }
    }
};

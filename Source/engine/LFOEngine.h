#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <cstdlib>

// ─── LFOEngine ───────────────────────────────────────────────────────────────
//  Block-rate LFO (tick() called once per audio block).
//  Shapes: 0=Sine 1=Triangle 2=Saw 3=RevSaw 4=Square 5=SampleAndHold 6=SmoothRandom
class LFOEngine {
public:
    void prepare(double sampleRate, int blockSize = 512) {
        sampleRate_ = sampleRate;
        blockSize_  = blockSize;
        phase_      = 0.f;
        shValue_    = 0.f;
        smoothed_   = 0.f;
    }

    void setShape(int s)     { shape_ = juce::jlimit(0, 6, s); }
    void setRate (float hz)  { rate_  = hz; }
    void setDepth(float d)   { depth_ = d; }
    void setPhase(float p01) { phaseOffset_ = p01; }

    // Call once per audio block; returns modulation value −depth..+depth
    float tick() {
        // Phase increment scaled to block rate
        const float inc = rate_ * (float)blockSize_ / (float)sampleRate_;
        phase_ += inc;
        if (phase_ >= 1.f) {
            phase_ -= 1.f;
            if (shape_ == 5 || shape_ == 6) {
                shValue_ = (float)std::rand() / (float)RAND_MAX * 2.f - 1.f;
            }
        }

        const float p = std::fmod(phase_ + phaseOffset_, 1.f);
        float out = 0.f;

        switch (shape_) {
            case 0: // Sine
                out = std::sin(p * juce::MathConstants<float>::twoPi);
                break;
            case 1: // Triangle
                out = (p < 0.5f) ? (p * 4.f - 1.f) : (3.f - p * 4.f);
                break;
            case 2: // Saw
                out = p * 2.f - 1.f;
                break;
            case 3: // Rev Saw
                out = 1.f - p * 2.f;
                break;
            case 4: // Square
                out = (p < 0.5f) ? 1.f : -1.f;
                break;
            case 5: // Sample & Hold
                out = shValue_;
                break;
            case 6: // Smooth Random — slew-limited S&H
            {
                float fc = juce::jlimit(0.001f, 0.99f,
                                        rate_ * 4.f * (float)blockSize_ / (float)sampleRate_);
                smoothed_ += fc * (shValue_ - smoothed_);
                out = smoothed_;
                break;
            }
            default: break;
        }

        return out * depth_;
    }

private:
    double sampleRate_  = 44100.0;
    int    blockSize_   = 512;
    float  rate_        = 1.f;
    float  depth_       = 1.f;
    float  phaseOffset_ = 0.f;
    int    shape_       = 0;
    float  phase_       = 0.f;
    float  shValue_     = 0.f;
    float  smoothed_    = 0.f;
};

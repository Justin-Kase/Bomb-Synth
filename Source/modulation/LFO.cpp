#include "LFO.h"
#include <cmath>

static constexpr float kTwoPi = 6.28318530718f;

void LFO::prepare(double sr) { sampleRate_ = sr; setRate(rate_); }

void LFO::noteOn() {
    if (mode_ == LFOMode::Retrig || mode_ == LFOMode::OneShot) phase_ = 0.f;
    fadeTimer_ = 0.f;
}

void LFO::setRate(float hz) {
    rate_     = hz;
    phaseInc_ = hz / (float)sampleRate_;
}

void LFO::setTempoSync(bool /*sync*/, double bpm, float beatsPerCycle) {
    float hz = (float)(bpm / 60.0) / beatsPerCycle;
    setRate(hz);
}

float LFO::sampleWaveform(float phase) const {
    switch (shape_) {
        case LFOShape::Sine:     return std::sin(phase * kTwoPi);
        case LFOShape::Triangle: return 1.f - 4.f * std::abs(phase - 0.5f);
        case LFOShape::Saw:      return 2.f * phase - 1.f;
        case LFOShape::RevSaw:   return 1.f - 2.f * phase;
        case LFOShape::Square:   return phase < 0.5f ? 1.f : -1.f;
        case LFOShape::Custom:
            if (!customTable_.empty()) {
                float idx = phase * (float)(customTable_.size() - 1);
                int   i0  = (int)idx;
                int   i1  = std::min(i0 + 1, (int)customTable_.size() - 1);
                float a   = idx - (float)i0;
                return customTable_[i0] + a * (customTable_[i1] - customTable_[i0]);
            }
            return 0.f;
        default: return 0.f;
    }
}

float LFO::sampleSmoothRandom() {
    if (phase_ < shSmooth_) {
        shValue_ += (shTarget_ - shValue_) * 0.01f;
    } else {
        shSmooth_ = phase_ + 0.1f;
        shTarget_ = rng_.nextFloat() * 2.f - 1.f;
    }
    return shValue_;
}

float LFO::process() {
    float val = 0.f;
    float ph  = std::fmod(phase_ + phaseOffset_, 1.f);

    if (shape_ == LFOShape::SampleHold) {
        if (phase_ < phaseInc_) shValue_ = rng_.nextFloat() * 2.f - 1.f;
        val = shValue_;
    } else if (shape_ == LFOShape::SmoothRandom) {
        val = sampleSmoothRandom();
    } else {
        val = sampleWaveform(ph);
    }

    // Fade-in
    if (fadeIn_ > 0.f) {
        float fade = juce::jlimit(0.f, 1.f, fadeTimer_ / (fadeIn_ * (float)sampleRate_));
        val *= fade;
        fadeTimer_ += 1.f;
    }

    phase_ += phaseInc_;
    if (phase_ >= 1.f) {
        phase_ -= 1.f;
        if (mode_ == LFOMode::OneShot) { phase_ = 0.f; phaseInc_ = 0.f; }
    }

    return val * depth_;
}

void LFO::processBlock(float* buf, int n) {
    for (int i = 0; i < n; ++i) buf[i] = process();
}

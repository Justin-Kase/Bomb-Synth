#include "AnalogOscillator.h"
#include <cmath>

static constexpr float kTwoPi = 6.28318530718f;

void AnalogOscillator::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    reset();
}

void AnalogOscillator::reset() {
    phase_ = 0.f;
    fmInput_ = 0.f;
    driftValue_ = 0.f;
    for (auto& p : unisonPhases_) p = rng_.nextFloat();
    setFrequency(frequency_);
}

void AnalogOscillator::setFrequency(float hz) {
    frequency_ = hz;
    phaseInc_  = hz / (float)sampleRate_;

    for (int i = 0; i < unisonCount_; ++i) {
        float semis = (unisonCount_ == 1) ? 0.f
            : unisonDetune_ * (2.f * i / (float)(unisonCount_ - 1) - 1.f);
        float detunedHz = hz * std::pow(2.f, semis / 12.f);
        unisonPhaseIncs_[i] = detunedHz / (float)sampleRate_;
    }
}

void AnalogOscillator::setUnisonVoices(int n, float detune, float spread) {
    unisonCount_  = juce::jlimit(1, kMaxUnison, n);
    unisonDetune_ = detune;
    unisonSpread_ = spread;
    for (int i = 0; i < unisonCount_; ++i)
        unisonPans_[i] = (unisonCount_ == 1) ? 0.f
            : spread * (2.f * i / (float)(unisonCount_ - 1) - 1.f);
    setFrequency(frequency_);
}

// PolyBLEP correction term
float AnalogOscillator::polyBLEP(float t, float dt) const {
    if (t < dt) {
        t /= dt; return t + t - t * t - 1.f;
    } else if (t > 1.f - dt) {
        t = (t - 1.f) / dt; return t * t + t + t + 1.f;
    }
    return 0.f;
}

float AnalogOscillator::processSingle(float phase, float dt) const {
    float out = 0.f;
    switch (waveform_) {
        case OscWaveform::Sine:
            out = std::sin(phase * kTwoPi);
            break;
        case OscWaveform::Saw:
            out = 2.f * phase - 1.f;
            out -= polyBLEP(phase, dt);
            break;
        case OscWaveform::Square: {
            out = phase < pulseWidth_ ? 1.f : -1.f;
            out += polyBLEP(phase, dt);
            out -= polyBLEP(std::fmod(phase + 1.f - pulseWidth_, 1.f), dt);
            break;
        }
        case OscWaveform::Triangle:
            out = 2.f * phase - 1.f;
            out -= polyBLEP(phase, dt);
            // Integrate leaky: triangle from saw
            out = 2.f * std::abs(2.f * phase - 1.f) - 1.f;
            break;
        case OscWaveform::SawTri:
            out = (2.f * phase - 1.f) * 0.5f
                + (2.f * std::abs(2.f * phase - 1.f) - 1.f) * 0.5f;
            out -= polyBLEP(phase, dt) * 0.5f;
            break;
        case OscWaveform::Noise:
            // White noise — phase ignored; uses rng_ at call site
            out = 0.f;
            break;
    }
    return out;
}

void AnalogOscillator::updateDrift() {
    if (drift_ <= 0.f) { driftValue_ = 0.f; return; }
    // Random walk, smoothed
    driftValue_ += (rng_.nextFloat() * 2.f - 1.f) * 0.01f * drift_;
    driftValue_ = juce::jlimit(-drift_, drift_, driftValue_);
}

float AnalogOscillator::process() {
    if (waveform_ == OscWaveform::Noise)
        return (rng_.nextFloat() * 2.f - 1.f) * gain_;

    updateDrift();
    float driftHz = frequency_ * (std::pow(2.f, driftValue_ / 1200.f) - 1.f);
    float fmHz    = fmAmount_ * fmInput_ * frequency_;
    float totalInc = phaseInc_ + (driftHz + fmHz) / (float)sampleRate_;

    float out = 0.f;
    if (unisonCount_ == 1) {
        out = processSingle(phase_, totalInc);
        phase_ = std::fmod(phase_ + totalInc, 1.f);
    } else {
        for (int i = 0; i < unisonCount_; ++i) {
            out += processSingle(unisonPhases_[i], unisonPhaseIncs_[i]);
            unisonPhases_[i] = std::fmod(unisonPhases_[i] + unisonPhaseIncs_[i], 1.f);
        }
        out /= (float)unisonCount_;
    }
    return out;
}

void AnalogOscillator::processBlock(float* out, int numSamples) {
    for (int i = 0; i < numSamples; ++i)
        out[i] = process();
}

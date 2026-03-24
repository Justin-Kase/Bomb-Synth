#include "WavetableOscillator.h"
#include <cmath>
#include <algorithm>

void WavetableOscillator::prepare(double sr) { sampleRate_ = sr; reset(); }

void WavetableOscillator::reset() { phase_ = 0.f; }

void WavetableOscillator::setFrequency(float hz) {
    frequency_ = hz;
    phaseInc_  = hz / (float)sampleRate_;
}

void WavetableOscillator::loadFrames(const std::vector<std::vector<float>>& frames) {
    frames_ = frames;
}

void WavetableOscillator::setMorphPosition(float pos) {
    if (!frames_.empty())
        morphPos_ = juce::jlimit(0.f, (float)(frames_.size() - 1), pos);
}

float WavetableOscillator::interpolateSample(int fA, int fB, float frac, float phase) const {
    if (frames_.empty()) return 0.f;
    fA = juce::jlimit(0, (int)frames_.size() - 1, fA);
    fB = juce::jlimit(0, (int)frames_.size() - 1, fB);

    float idx   = phase * (float)kFrameSize;
    int   i0    = (int)idx % kFrameSize;
    int   i1    = (i0 + 1) % kFrameSize;
    float alpha = idx - (int)idx;

    float sA = frames_[fA][i0] + alpha * (frames_[fA][i1] - frames_[fA][i0]);
    float sB = frames_[fB][i0] + alpha * (frames_[fB][i1] - frames_[fB][i0]);
    return sA + frac * (sB - sA);
}

float WavetableOscillator::process() {
    if (frames_.empty()) return 0.f;

    int   frameA = (int)morphPos_;
    int   frameB = std::min(frameA + 1, (int)frames_.size() - 1);
    float frac   = morphPos_ - (float)frameA;

    float out = interpolateSample(frameA, frameB, frac, phase_);
    phase_ = std::fmod(phase_ + phaseInc_, 1.f);
    return out;
}

void WavetableOscillator::processBlock(float* out, int n) {
    for (int i = 0; i < n; ++i) out[i] = process();
}

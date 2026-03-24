#include "SVFFilter.h"
#include <cmath>

void SVFFilter::prepare(double sr) { sampleRate_ = sr; reset(); setCutoff(cutoff_); }
void SVFFilter::reset() { ic1eq_ = 0.f; ic2eq_ = 0.f; }

void SVFFilter::setCutoff(float hz) {
    cutoff_ = juce::jlimit(20.f, 20000.f, hz);
    g_  = std::tan((float)juce::MathConstants<double>::pi * cutoff_ / (float)sampleRate_);
    k_  = 2.f - 2.f * q_;
    a1_ = 1.f / (1.f + g_ * (g_ + k_));
    a2_ = g_ * a1_;
    a3_ = g_ * a2_;
}

void SVFFilter::setResonance(float q) { q_ = juce::jlimit(0.f, 1.f, q); setCutoff(cutoff_); }

float SVFFilter::processSample(float v0) {
    float v3 = v0 - ic2eq_;
    float v1 = a1_ * ic1eq_ + a2_ * v3;
    float v2 = ic2eq_ + a2_ * ic1eq_ + a3_ * v3;
    ic1eq_ = 2.f * v1 - ic1eq_;
    ic2eq_ = 2.f * v2 - ic2eq_;

    switch (mode_) {
        case SVFMode::LP:      return v2;
        case SVFMode::HP:      return v0 - k_ * v1 - v2;
        case SVFMode::BP:      return v1;
        case SVFMode::Notch:   return v0 - k_ * v1;
        case SVFMode::Peak:    return v2 - (v0 - k_ * v1 - v2);
        case SVFMode::Allpass: return v0 - 2.f * k_ * v1;
        default:               return v2;
    }
}

void SVFFilter::processBlock(float* buf, int n) {
    for (int i = 0; i < n; ++i) buf[i] = processSample(buf[i]);
}

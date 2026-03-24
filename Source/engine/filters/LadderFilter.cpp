#include "LadderFilter.h"
#include <cmath>

void LadderFilter::prepare(double sr, int blockSize) {
    sampleRate_ = sr;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr * 2.0;  // 2x oversampled
    spec.maximumBlockSize = (juce::uint32)(blockSize * 2);
    spec.numChannels      = 1;
    oversampler_.initProcessing((size_t)blockSize);
    reset();
    setCutoff(cutoff_);
}

void LadderFilter::reset() {
    for (auto& s : s_) s = 0.f;
    oversampler_.reset();
}

void LadderFilter::setCutoff(float hz) {
    cutoff_ = juce::jlimit(20.f, 20000.f, hz);
    // Bilinear-warped coefficient at 2x sample rate
    float wc = (float)(2.0 * juce::MathConstants<double>::pi * cutoff_ / (sampleRate_ * 2.0));
    g_ = wc / (1.f + wc);
}

void LadderFilter::setResonance(float r) {
    resonance_ = juce::jlimit(0.f, 1.f, r);
    k_ = resonance_ * 4.f;   // 4 = max resonance for 4-pole
}

// Fast tanh approximation (Padé)
float LadderFilter::tanh_approx(float x) const {
    float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}

float LadderFilter::processSample(float in) {
    // Huovilainen ladder model
    float fb = s_[3] * k_;
    float inp = tanh_approx((in * drive_ - fb) * 0.25f);

    float g1 = g_ * (inp  - s_[0]); s_[0] += g1 + g1;
    float g2 = g_ * (s_[0] - s_[1]); s_[1] += g2 + g2;
    float g3 = g_ * (s_[1] - s_[2]); s_[2] += g3 + g3;
    float g4 = g_ * (s_[2] - s_[3]); s_[3] += g4 + g4;

    switch (mode_) {
        case LadderMode::LP24: return s_[3];
        case LadderMode::LP12: return s_[1];
        case LadderMode::HP24: return inp - k_ * s_[3] - 4.f * s_[0] + 6.f * s_[1] - 4.f * s_[2] + s_[3];
        case LadderMode::HP12: return inp - s_[0] - s_[1];
        case LadderMode::BP:   return 4.f * (s_[1] - 2.f * s_[2] + s_[3]);
        default:                return s_[3];
    }
}

void LadderFilter::processBlock(float* buf, int numSamples) {
    // Use JUCE oversampler for 2× processing
    juce::AudioBuffer<float> mono(1, numSamples);
    mono.copyFrom(0, 0, buf, numSamples);
    juce::dsp::AudioBlock<float> block(mono);

    auto upBlock = oversampler_.processSamplesUp(block);
    float* up = upBlock.getChannelPointer(0);
    for (int i = 0; i < (int)upBlock.getNumSamples(); ++i)
        up[i] = processSample(up[i]);
    oversampler_.processSamplesDown(block);

    juce::FloatVectorOperations::copy(buf, mono.getReadPointer(0), numSamples);
}

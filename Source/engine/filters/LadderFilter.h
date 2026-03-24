#pragma once
#include <juce_dsp/juce_dsp.h>

enum class LadderMode { LP24, LP12, HP24, HP12, BP };

class LadderFilter {
public:
    void prepare(double sampleRate, int blockSize);
    void setCutoff(float hz);
    void setResonance(float r);   // 0..1 (>0.9 = self-oscillation)
    void setMode(LadderMode m)   { mode_ = m; }
    void setDrive(float d)       { drive_ = d; }
    void reset();

    float processSample(float in);
    void  processBlock(float* buf, int numSamples);

private:
    float tanh_approx(float x) const;

    double sampleRate_  = 44100.0;
    float  cutoff_      = 1000.f;
    float  resonance_   = 0.f;
    float  drive_       = 1.f;
    LadderMode mode_    = LadderMode::LP24;

    // Internal state (4 one-pole stages)
    float s_[4]     = {};
    float g_        = 0.f;   // cutoff coefficient
    float k_        = 0.f;   // resonance coefficient

    // 2x oversampling
    juce::dsp::Oversampling<float> oversampler_ { 1, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
};

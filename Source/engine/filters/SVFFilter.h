#pragma once
#include <juce_dsp/juce_dsp.h>

enum class SVFMode { LP, HP, BP, Notch, Peak, Allpass };

// State-Variable Filter (SEM/OTA-style 2-pole)
class SVFFilter {
public:
    void  prepare(double sampleRate);
    void  setCutoff(float hz);
    void  setResonance(float q);    // 0..1
    void  setMode(SVFMode m)  { mode_ = m; }
    void  reset();
    float processSample(float in);
    void  processBlock(float* buf, int numSamples);

private:
    double sampleRate_ = 44100.0;
    float  cutoff_     = 1000.f;
    float  q_          = 0.5f;
    SVFMode mode_      = SVFMode::LP;
    float  ic1eq_      = 0.f;
    float  ic2eq_      = 0.f;
    float  g_          = 0.f;
    float  k_          = 0.f;
    float  a1_         = 0.f, a2_ = 0.f, a3_ = 0.f;
};

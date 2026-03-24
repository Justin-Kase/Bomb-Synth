#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

enum class ADSRStage { Idle, Attack, Decay, Sustain, Release };

class ADSR {
public:
    struct Params {
        float attackMs  = 5.f;
        float decayMs   = 100.f;
        float sustain   = 0.7f;   // 0..1 level
        float releaseMs = 300.f;
        float curve     = 0.f;    // -1=log, 0=lin, 1=exp
    };

    void prepare(double sampleRate);
    void setParams(const Params& p);
    void noteOn();
    void noteOff();
    void reset();

    float process();
    void  processBlock(float* buf, int numSamples);
    bool  isIdle() const { return stage_ == ADSRStage::Idle; }
    float getLevel() const { return level_; }

private:
    float applyShape(float t) const;   // apply curve to linear 0..1

    double sampleRate_  = 44100.0;
    Params params_;
    ADSRStage stage_    = ADSRStage::Idle;
    float     level_    = 0.f;
    float     attackInc_  = 0.f;
    float     decayInc_   = 0.f;
    float     releaseInc_ = 0.f;
};

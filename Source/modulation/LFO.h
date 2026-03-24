#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <functional>

enum class LFOShape { Sine, Triangle, Saw, RevSaw, Square, SampleHold, SmoothRandom, Custom };
enum class LFOMode  { Free, Retrig, OneShot };

class LFO {
public:
    void  prepare(double sampleRate);
    void  setRate(float hz);
    void  setTempoSync(bool sync, double bpm, float beatsPerCycle);
    void  setShape(LFOShape s)     { shape_ = s; }
    void  setMode(LFOMode m)       { mode_ = m; }
    void  setDepth(float d)        { depth_ = d; }
    void  setPhaseOffset(float p)  { phaseOffset_ = p; }
    void  setFadeIn(float secs)    { fadeIn_ = secs; }
    void  setCustomShape(const std::vector<float>& pts) { customTable_ = pts; }

    void  noteOn();
    float process();
    void  processBlock(float* buf, int numSamples);

private:
    float sampleWaveform(float phase) const;
    float sampleSmoothRandom();

    double sampleRate_   = 44100.0;
    float  rate_         = 1.f;
    float  phase_        = 0.f;
    float  phaseInc_     = 0.f;
    float  phaseOffset_  = 0.f;
    float  depth_        = 1.f;
    float  fadeIn_       = 0.f;
    float  fadeTimer_    = 0.f;
    LFOShape shape_      = LFOShape::Sine;
    LFOMode  mode_       = LFOMode::Free;

    // S&H / smooth random
    float  shValue_      = 0.f;
    float  shTarget_     = 0.f;
    float  shSmooth_     = 0.f;

    std::vector<float> customTable_;
    juce::Random rng_;
};

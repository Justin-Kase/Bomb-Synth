#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

enum class SpectralWarpMode { None, PhaseBend, Smear, Mirror };

class WavetableOscillator {
public:
    static constexpr int kFrameSize = 2048;

    void prepare(double sampleRate);
    void loadFrames(const std::vector<std::vector<float>>& frames);
    void setFrequency(float hz);
    void setMorphPosition(float pos);   // 0..numFrames-1
    void setWarpMode(SpectralWarpMode m) { warpMode_ = m; }
    void setWarpAmount(float a)          { warpAmount_ = a; }
    void reset();

    float process();
    void  processBlock(float* out, int numSamples);

private:
    float interpolateSample(int frameA, int frameB, float frameFrac, float phase) const;

    double sampleRate_   = 44100.0;
    float  frequency_    = 440.f;
    float  phase_        = 0.f;
    float  phaseInc_     = 0.f;
    float  morphPos_     = 0.f;
    float  warpAmount_   = 0.f;
    SpectralWarpMode warpMode_ = SpectralWarpMode::None;

    std::vector<std::vector<float>> frames_;   // [frame][sample]
};

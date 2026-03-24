#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>

struct Grain {
    float position   = 0.f;   // playback position in sample buffer (samples)
    float size       = 0.f;   // grain size in samples
    float playhead   = 0.f;   // current sample within grain
    float pitchRatio = 1.f;
    float amplitude  = 1.f;
    float pan        = 0.f;   // -1..1
    bool  active     = false;
};

class GranularEngine {
public:
    static constexpr int kMaxGrains = 64;

    void prepare(double sampleRate);
    void loadSample(const juce::AudioBuffer<float>& buf);
    void setPosition(float pos)     { position_  = pos;  }   // 0..1 in buffer
    void setDensity(float gps)      { density_   = gps;  }   // grains/second
    void setGrainSize(float ms)     { grainSize_ = ms;   }   // milliseconds
    void setSpray(float spray)      { spray_     = spray;}   // position randomness 0..1
    void setPitchScatter(float st)  { pitchScat_ = st;   }   // semitones
    void setFrequency(float hz)     { frequency_ = hz;   }
    void noteOn();
    void noteOff();
    void reset();

    void processBlock(float* outL, float* outR, int numSamples);

private:
    void  spawnGrain();
    float hannWindow(float phase) const;   // phase 0..1

    double sampleRate_  = 44100.0;
    float  position_    = 0.f;
    float  density_     = 20.f;
    float  grainSize_   = 80.f;
    float  spray_       = 0.1f;
    float  pitchScat_   = 0.f;
    float  frequency_   = 440.f;
    float  spawnTimer_  = 0.f;
    bool   active_      = false;

    juce::AudioBuffer<float> sampleBuffer_;
    std::array<Grain, kMaxGrains> grains_ {};
    juce::Random rng_;
};

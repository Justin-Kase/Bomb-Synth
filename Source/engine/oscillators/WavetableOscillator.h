#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "WavetableBank.h"

class WavetableOscillator {
public:
    void prepare(double sampleRate);
    void reset();

    // Bank selection — loads pointer from WavetableBankLibrary
    void setBankIndex(int idx);
    int  getBankIndex() const { return bankIdx_; }

    void setFrequency(float hz);
    void setMorphPosition(float pos01);   // 0 = frame 0, 1 = frame 7
    void setGain(float g)  { gain_ = g;  }

    // Legacy frame-load interface (copies into local storage)
    void loadFrames(const std::vector<std::vector<float>>& frames);

    float process();
    void  processBlock(float* out, int numSamples);

private:
    const WavetableBank* currentBank_ = nullptr;

    // Local bank used when loadFrames() is called (not from library)
    WavetableBank localBank_;
    bool usingLocalBank_ = false;

    double sampleRate_  = 44100.0;
    float  frequency_   = 440.f;
    double phase_       = 0.0;   // [0, 1)
    double phaseInc_    = 0.0;
    float  morphPos_    = 0.f;   // [0, 1]
    float  gain_        = 1.f;
    int    bankIdx_     = 0;
};

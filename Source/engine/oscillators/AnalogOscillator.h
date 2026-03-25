#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

enum class OscWaveform { Sine, Saw, Square, Triangle, SawTri, Noise };

// Engine indices (stored in osc{n}_engine APVTS param)
namespace OscEngine {
    enum { WT=0, GR, Sine, Saw, SuperSaw, Square, Triangle, SawTri, Noise, Count };
    static constexpr const char* kNames[] = {
        "WT","GR","SINE","SAW","SPSAW","SQR","TRI","SAWTRI","NOISE"
    };
}

class AnalogOscillator {
public:
    void prepare(double sampleRate);
    void setFrequency(float hz);
    void setWaveform(OscWaveform w)    { waveform_ = w; }
    void setSuperSaw(float detuneAmt) {    // detuneAmt 0-1 → 0-50 cents spread
        setWaveform(OscWaveform::Saw);
        setUnisonVoices(7, detuneAmt * 50.f, 1.0f);
    }
    void resetUnison()                { setUnisonVoices(1, 0.f, 0.f); }
    void setPulseWidth(float pw)       { pulseWidth_ = juce::jlimit(0.05f, 0.95f, pw); }
    void setFMAmount(float amt)        { fmAmount_ = amt; }
    void setFMInput(float sample)      { fmInput_ = sample; }
    void setDrift(float cents)         { drift_ = cents; }
    void setGain(float g)              { gain_ = g; }
    void setUnisonVoices(int n, float detune, float spread);
    void reset();

    float process();                   // returns one sample
    void  processBlock(float* out, int numSamples);

private:
    float polyBLEP(float t, float dt) const;
    float processSingle(float phase, float dt) const;
    void  updateDrift();

    double sampleRate_   = 44100.0;
    float  frequency_    = 440.f;
    float  phase_        = 0.f;
    float  phaseInc_     = 0.f;
    float  pulseWidth_   = 0.5f;
    float  fmAmount_     = 0.f;
    float  fmInput_      = 0.f;
    float  gain_         = 1.f;
    float  drift_        = 0.f;        // cents
    float  driftValue_   = 0.f;        // current random-walk value
    OscWaveform waveform_ = OscWaveform::Saw;

    // Unison
    static constexpr int kMaxUnison = 8;
    int   unisonCount_   = 1;
    float unisonDetune_  = 0.f;
    float unisonSpread_  = 0.f;
    std::array<float, kMaxUnison> unisonPhases_     {};
    std::array<float, kMaxUnison> unisonPhaseIncs_  {};
    std::array<float, kMaxUnison> unisonPans_       {};

    juce::Random rng_;
};

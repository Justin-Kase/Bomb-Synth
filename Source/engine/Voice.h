#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "oscillators/AnalogOscillator.h"
#include "oscillators/WavetableOscillator.h"
#include "oscillators/GranularEngine.h"
#include "filters/LadderFilter.h"
#include "filters/SVFFilter.h"
#include "envelopes/ADSR.h"

enum class OscEngineType { Analog, Wavetable, Granular };
enum class FilterRouting  { Serial, Parallel };

class Voice {
public:
    static constexpr int kNumOscs = 3;

    void prepare(double sampleRate, int blockSize);
    void noteOn (int midiNote, float velocity);
    void noteOff();
    void reset();

    bool isActive() const;
    void renderBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    // Parameter setters
    void setOscEngine(int idx, OscEngineType t);
    void setAmpEnvParams(const ADSR::Params& p)    { ampEnv_.setParams(p); }
    void setFilterEnvParams(const ADSR::Params& p) { filterEnv_.setParams(p); }
    void setCutoff(float hz)     { ladderFilter_.setCutoff(hz);  svfFilter_.setCutoff(hz); }
    void setResonance(float r)   { ladderFilter_.setResonance(r); svfFilter_.setResonance(r); }
    void setFilterRouting(FilterRouting r) { filterRouting_ = r; }
    void setPan(float pan)       { pan_ = pan; }
    void setGain(float g)        { gain_ = g; }

    // Wavetable control
    void setOscBankIndex (int oscIdx, int bankIdx);
    void setOscMorphPos  (int oscIdx, float morph01);
    void setOscLevel     (int oscIdx, float level);
    void setOscTune      (int oscIdx, float semitones);  // coarse detune

    int   getMidiNote() const { return midiNote_; }
    float getVelocity() const { return velocity_; }

private:
    float midiNoteToHz(int note, float tuneOffset = 0.f) const;

    double sampleRate_  = 44100.0;
    int    blockSize_   = 512;
    int    midiNote_    = 60;
    float  velocity_    = 1.f;
    float  pan_         = 0.f;
    float  gain_        = 1.f;
    FilterRouting filterRouting_ = FilterRouting::Serial;

    std::array<OscEngineType, kNumOscs> oscTypes_ {
        OscEngineType::Wavetable,
        OscEngineType::Wavetable,
        OscEngineType::Wavetable
    };
    std::array<float, kNumOscs> oscLevels_ { 1.f, 0.f, 0.f };
    std::array<float, kNumOscs> oscTune_   { 0.f, 0.f, 0.f }; // semitones
    std::array<int,   kNumOscs> oscBanks_  { -1, -1, -1 };     // -1 = needs init

    AnalogOscillator    analogOscs_[kNumOscs];
    WavetableOscillator wavetableOscs_[kNumOscs];

    LadderFilter ladderFilter_;
    SVFFilter    svfFilter_;

    ADSR ampEnv_;
    ADSR filterEnv_;

    juce::AudioBuffer<float> oscBuf_;
};

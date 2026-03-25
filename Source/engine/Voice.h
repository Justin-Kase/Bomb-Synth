#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "oscillators/AnalogOscillator.h"
#include "oscillators/WavetableOscillator.h"
#include "oscillators/GranularEngine.h"
#include "filters/LadderFilter.h"
#include "filters/SVFFilter.h"
#include "filters/FormantFilter.h"
#include "filters/CombFilter.h"
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
    void setCutoff(float hz)     { baseCutoff_ = hz; ladderFilter_.setCutoff(hz); svfFilter_.setCutoff(hz); combFilter_.setCutoff(hz); }
    void setResonance(float r)   { baseResonance_ = r; ladderFilter_.setResonance(r); svfFilter_.setResonance(r); }
    void setFilterRouting(FilterRouting r) { filterRouting_ = r; }
    void setFilterType(int t)    { filterType_ = t; }
    void setPan(float pan)       { pan_ = pan; }
    void setGain(float g)        { gain_ = g; }

    // Wavetable control
    void setOscBankIndex (int oscIdx, int bankIdx);
    void setOscMorphPos  (int oscIdx, float morph01);
    void setOscLevel     (int oscIdx, float level);
    void setOscTune      (int oscIdx, float semitones);  // coarse detune

    // Granular params
    void setGranularDensity  (int osc, float v) { if (osc>=0&&osc<kNumOscs) granParams_[osc].density   = v; }
    void setGranularSize     (int osc, float v) { if (osc>=0&&osc<kNumOscs) granParams_[osc].size      = v; }
    void setGranularSpray    (int osc, float v) { if (osc>=0&&osc<kNumOscs) granParams_[osc].spray     = v; }
    void setGranularPitchScat(int osc, float v) { if (osc>=0&&osc<kNumOscs) granParams_[osc].pitchScat = v; }

    // Warp params (forwarded to WavetableOscillator)
    void setOscWarpMode  (int osc, int mode)  { if (osc>=0&&osc<kNumOscs) wavetableOscs_[osc].setWarpMode  (static_cast<WarpMode>(mode)); }
    void setOscWarpAmount(int osc, float amt) { if (osc>=0&&osc<kNumOscs) wavetableOscs_[osc].setWarpAmount(amt); }

    // Modulation inputs (applied each renderBlock)
    void setModPitch(float semitones) { modPitch_ = semitones; }
    void setModMorph(int osc, float delta) {
        if (osc >= 0 && osc < kNumOscs) modMorph_[osc] = delta;
    }
    void setModAmp(float delta)       { modAmp_ = delta; }

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
    int    filterType_  = 0;          // 0=LadderLP24 1=LadderLP12 2=LadderHP 3=SVFLP 4=Formant 5=Comb
    float  baseCutoff_     = 6000.f;
    float  baseResonance_  = 0.f;
    FilterRouting filterRouting_ = FilterRouting::Serial;

    // Modulation offsets (set each block by SynthEngine)
    float modPitch_ = 0.f;
    float modAmp_   = 0.f;
    std::array<float, kNumOscs> modMorph_ { 0.f, 0.f, 0.f };
    std::array<float, kNumOscs> baseMorph_ { 0.f, 0.f, 0.f };

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
    GranularEngine      granularOscs_[kNumOscs];
    struct GranParams { float density=20, size=80, spray=0.1f, pitchScat=0; };
    std::array<GranParams, kNumOscs> granParams_;

    LadderFilter  ladderFilter_;
    SVFFilter     svfFilter_;
    FormantFilter formantFilter_;
    CombFilter    combFilter_;

    ADSR ampEnv_;
    ADSR filterEnv_;

    juce::AudioBuffer<float> oscBuf_;
};

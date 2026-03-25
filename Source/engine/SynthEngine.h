#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Voice.h"
#include <array>

class SynthEngine {
public:
    static constexpr int kMaxVoices = 16;

    void prepare(double sampleRate, int blockSize);
    void processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi);
    void reset();

    void setAmpEnvParams(const ADSR::Params& p);
    void setFilterEnvParams(const ADSR::Params& p);
    void setCutoff(float hz);
    void setResonance(float r);
    void setMasterGain(float g) { masterGain_ = g; }
    void setFilterType(int t);

    // Oscillator engine type
    void setOscEngine   (int oscIdx, OscEngineType t);
    void setOscEngineIdx(int oscIdx, int engineIdx)
        { for (auto& v : voices_) v.setOscEngineIdx(oscIdx, engineIdx); }

    // Wavetable control — forwarded to all voices
    void setOscBankIndex(int oscIdx, int bankIdx);
    void setOscMorphPos (int oscIdx, float morph01);
    void setOscLevel    (int oscIdx, float level);
    void setOscTune     (int oscIdx, float semitones);

    // Granular control
    void setGranularDensity  (int osc, float v);
    void setGranularSize     (int osc, float v);
    void setGranularSpray    (int osc, float v);
    void setGranularPitchScat(int osc, float v);

    // Warp control
    void setOscWarpMode  (int osc, int  mode);
    void setOscWarpAmount(int osc, float amt);

    // Modulation pass-through — applied each processBlock
    void setModCutoffHz      (float hz) { modCutoffHz_        = hz; }
    void setModPitchSemitones(float st) { modPitchSemitones_  = st; }
    void setModAmp           (float v)  { modAmp_             = v;  }
    void setModMorph         (int i, float v) { if(i>=0&&i<3) modMorph_[i]  = v; }
    void setModTune          (int i, float v) { if(i>=0&&i<3) modTune_[i]   = v; }
    void setModFine          (int i, float v) { if(i>=0&&i<3) modFine_[i]   = v; }
    void setModLevel         (int i, float v) { if(i>=0&&i<3) modLevel_[i]  = v; }
    void setModFM            (int i, float v) { if(i>=0&&i<3) modFM_[i]     = v; }
    void setModDetune        (int i, float v) { if(i>=0&&i<3) modDetune_[i] = v; }
    void setModFilterRes     (float v)  { modFilterRes_       = v;  }
    void setModFilterDrive   (float v)  { modFilterDrive_     = v;  }

private:
    void handleNoteOn (int note, float vel);
    void handleNoteOff(int note);
    Voice* findFreeVoice(int note);
    Voice* stealVoice(int note);

    double sampleRate_ = 44100.0;
    int    blockSize_  = 512;
    float  masterGain_ = 0.8f;
    float  baseCutoff_ = 6000.f;
    ADSR::Params ampParams_;
    ADSR::Params filterParams_;

    // Modulation state
    float modCutoffHz_       = 0.f;
    float modPitchSemitones_ = 0.f;
    float modAmp_            = 0.f;
    float modFilterRes_      = 0.f;
    float modFilterDrive_    = 0.f;
    float modMorph_[3]       = {};
    float modTune_[3]        = {};
    float modFine_[3]        = {};
    float modLevel_[3]       = {};
    float modFM_[3]          = {};
    float modDetune_[3]      = {};

    std::array<Voice, kMaxVoices> voices_;
};

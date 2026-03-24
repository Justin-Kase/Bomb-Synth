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

    // Wavetable control — forwarded to all voices
    void setOscBankIndex(int oscIdx, int bankIdx);
    void setOscMorphPos (int oscIdx, float morph01);
    void setOscLevel    (int oscIdx, float level);
    void setOscTune     (int oscIdx, float semitones);

private:
    void handleNoteOn (int note, float vel);
    void handleNoteOff(int note);
    Voice* findFreeVoice(int note);
    Voice* stealVoice(int note);

    double sampleRate_ = 44100.0;
    int    blockSize_  = 512;
    float  masterGain_ = 0.8f;
    ADSR::Params ampParams_;
    ADSR::Params filterParams_;

    std::array<Voice, kMaxVoices> voices_;
};

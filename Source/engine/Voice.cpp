#include "Voice.h"
#include <cmath>

void Voice::prepare(double sr, int blockSize) {
    sampleRate_ = sr;
    blockSize_  = blockSize;
    for (auto& o : analogOscs_)    o.prepare(sr);
    for (auto& o : wavetableOscs_) o.prepare(sr);
    ladderFilter_.prepare(sr, blockSize);
    svfFilter_.prepare(sr);
    ampEnv_.prepare(sr);
    filterEnv_.prepare(sr);
    oscBuf_.setSize(1, blockSize);
    // Ensure all wavetable oscs have a bank loaded
    for (int i = 0; i < kNumOscs; ++i)
        if (oscBanks_[i] < 0) { oscBanks_[i] = 0; wavetableOscs_[i].setBankIndex(0); }
}

float Voice::midiNoteToHz(int note, float tuneOffset) const {
    return 440.f * std::pow(2.f, (note - 69 + tuneOffset) / 12.f);
}

void Voice::noteOn(int midiNote, float velocity) {
    midiNote_ = midiNote;
    velocity_ = velocity;
    for (int i = 0; i < kNumOscs; ++i) {
        float hz = midiNoteToHz(midiNote, oscTune_[i]);
        analogOscs_[i].setFrequency(hz);
        wavetableOscs_[i].setFrequency(hz);
    }
    ampEnv_.noteOn();
    filterEnv_.noteOn();
}

void Voice::noteOff() {
    ampEnv_.noteOff();
    filterEnv_.noteOff();
}

void Voice::reset() {
    for (auto& o : analogOscs_)    o.reset();
    for (auto& o : wavetableOscs_) o.reset();
    ampEnv_.reset();
    filterEnv_.reset();
}

bool Voice::isActive() const { return !ampEnv_.isIdle(); }

void Voice::setOscEngine(int idx, OscEngineType t) {
    if (idx >= 0 && idx < kNumOscs) oscTypes_[idx] = t;
}

void Voice::setOscBankIndex(int oscIdx, int bankIdx) {
    if (oscIdx < 0 || oscIdx >= kNumOscs) return;
    if (oscBanks_[oscIdx] == bankIdx)     return;   // no change
    oscBanks_[oscIdx] = bankIdx;
    wavetableOscs_[oscIdx].setBankIndex(bankIdx);
}

void Voice::setOscMorphPos(int oscIdx, float morph01) {
    if (oscIdx >= 0 && oscIdx < kNumOscs)
        wavetableOscs_[oscIdx].setMorphPosition(morph01);
}

void Voice::setOscLevel(int oscIdx, float level) {
    if (oscIdx >= 0 && oscIdx < kNumOscs) {
        oscLevels_[oscIdx] = level;
        wavetableOscs_[oscIdx].setGain(level);
        analogOscs_[oscIdx].setGain(level);
    }
}

void Voice::setOscTune(int oscIdx, float semitones) {
    if (oscIdx >= 0 && oscIdx < kNumOscs) {
        oscTune_[oscIdx] = semitones;
        if (isActive()) {
            float hz = midiNoteToHz(midiNote_, semitones);
            analogOscs_[oscIdx].setFrequency(hz);
            wavetableOscs_[oscIdx].setFrequency(hz);
        }
    }
}

void Voice::renderBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) {
    if (!isActive()) return;

    oscBuf_.setSize(1, numSamples, false, false, true);
    oscBuf_.clear();
    float* mono = oscBuf_.getWritePointer(0);

    float activeLevels = 0.f;
    for (int i = 0; i < kNumOscs; ++i) activeLevels += oscLevels_[i];
    float normGain = (activeLevels > 0.f) ? 1.f / activeLevels : 1.f;

    for (int i = 0; i < kNumOscs; ++i) {
        if (oscLevels_[i] < 1e-4f) continue;

        std::vector<float> tmp(numSamples);
        switch (oscTypes_[i]) {
            case OscEngineType::Wavetable:
                wavetableOscs_[i].processBlock(tmp.data(), numSamples);
                break;
            case OscEngineType::Analog:
                analogOscs_[i].processBlock(tmp.data(), numSamples);
                break;
            default: break;
        }
        juce::FloatVectorOperations::add(mono, tmp.data(), numSamples);
    }

    juce::FloatVectorOperations::multiply(mono, normGain, numSamples);

    // Filter
    if (filterRouting_ == FilterRouting::Serial) {
        ladderFilter_.processBlock(mono, numSamples);
    } else {
        std::vector<float> ladderOut(mono, mono + numSamples);
        std::vector<float> svfOut   (mono, mono + numSamples);
        ladderFilter_.processBlock(ladderOut.data(), numSamples);
        svfFilter_   .processBlock(svfOut   .data(), numSamples);
        for (int s = 0; s < numSamples; ++s)
            mono[s] = ladderOut[s] * 0.5f + svfOut[s] * 0.5f;
    }

    // Amp envelope
    for (int s = 0; s < numSamples; ++s)
        mono[s] *= ampEnv_.process() * velocity_ * gain_;

    // Stereo pan (constant power)
    float angle = (pan_ + 1.f) * 0.5f * 1.57079632f;
    float panL  = std::cos(angle);
    float panR  = std::sin(angle);

    auto* L = buffer.getWritePointer(0, startSample);
    auto* R = buffer.getWritePointer(1, startSample);
    for (int s = 0; s < numSamples; ++s) {
        L[s] += mono[s] * panL;
        R[s] += mono[s] * panR;
    }
}

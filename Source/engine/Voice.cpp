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
}

float Voice::midiNoteToHz(int note) const {
    return 440.f * std::pow(2.f, (note - 69) / 12.f);
}

void Voice::noteOn(int midiNote, float velocity) {
    midiNote_ = midiNote;
    velocity_ = velocity;
    float hz  = midiNoteToHz(midiNote);
    for (auto& o : analogOscs_)    o.setFrequency(hz);
    for (auto& o : wavetableOscs_) o.setFrequency(hz);
    ampEnv_.noteOn();
    filterEnv_.noteOn();
}

void Voice::noteOff() {
    ampEnv_.noteOff();
    filterEnv_.noteOff();
}

void Voice::reset() {
    for (auto& o : analogOscs_) o.reset();
    ampEnv_.reset();
    filterEnv_.reset();
}

bool Voice::isActive() const { return !ampEnv_.isIdle(); }

void Voice::setOscEngine(int idx, OscEngineType t) {
    if (idx >= 0 && idx < kNumOscs) oscTypes_[idx] = t;
}

void Voice::renderBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) {
    if (!isActive()) return;

    oscBuf_.setSize(1, numSamples, false, false, true);
    oscBuf_.clear();
    float* mono = oscBuf_.getWritePointer(0);

    // Mix oscillators
    for (int i = 0; i < kNumOscs; ++i) {
        switch (oscTypes_[i]) {
            case OscEngineType::Analog: {
                std::vector<float> tmp(numSamples);
                analogOscs_[i].processBlock(tmp.data(), numSamples);
                juce::FloatVectorOperations::add(mono, tmp.data(), numSamples);
                break;
            }
            case OscEngineType::Wavetable: {
                std::vector<float> tmp(numSamples);
                wavetableOscs_[i].processBlock(tmp.data(), numSamples);
                juce::FloatVectorOperations::add(mono, tmp.data(), numSamples);
                break;
            }
            default: break;
        }
    }

    // Scale by num oscs
    juce::FloatVectorOperations::multiply(mono, 1.f / (float)kNumOscs, numSamples);

    // Filter (ladder in series with svf, or parallel)
    if (filterRouting_ == FilterRouting::Serial) {
        ladderFilter_.processBlock(mono, numSamples);
        svfFilter_.processBlock(mono, numSamples);
    } else {
        std::vector<float> svfBuf(mono, mono + numSamples);
        ladderFilter_.processBlock(mono, numSamples);
        svfFilter_.processBlock(svfBuf.data(), numSamples);
        juce::FloatVectorOperations::add(mono, svfBuf.data(), numSamples);
        juce::FloatVectorOperations::multiply(mono, 0.5f, numSamples);
    }

    // Amp envelope
    for (int s = 0; s < numSamples; ++s)
        mono[s] *= ampEnv_.process() * velocity_ * gain_;

    // Stereo panning (constant power)
    float panL = std::cos((pan_ + 1.f) * 0.5f * (float)juce::MathConstants<double>::pi * 0.5f);
    float panR = std::sin((pan_ + 1.f) * 0.5f * (float)juce::MathConstants<double>::pi * 0.5f);

    auto* L = buffer.getWritePointer(0, startSample);
    auto* R = buffer.getWritePointer(1, startSample);
    for (int s = 0; s < numSamples; ++s) {
        L[s] += mono[s] * panL;
        R[s] += mono[s] * panR;
    }
}

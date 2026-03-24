#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

// ─── CombFilter ──────────────────────────────────────────────────────────────
//  Feedback comb filter.  Delay time tracks cutoff frequency (delay = sr/hz).
class CombFilter {
public:
    void prepare(double sr) {
        sampleRate_ = sr;
        // Max delay: 1/20 Hz at given sample rate
        maxDelay_   = (int)(sr / 20.0) + 2;
        buffer_.assign(maxDelay_, 0.f);
        writePos_ = 0;
        setCutoff(440.f);
    }

    void setCutoff(float hz) {
        hz = juce::jmax(20.f, hz);
        delaySamples_ = juce::jlimit(1, maxDelay_ - 1,
                                     (int)(sampleRate_ / hz));
    }

    void setResonance(float r) {
        feedback_ = juce::jlimit(0.f, 0.99f, r);
    }

    float processSample(float x) {
        const int readPos = (writePos_ - delaySamples_ + maxDelay_) % maxDelay_;
        const float delayed = buffer_[readPos];
        const float out = x + delayed * feedback_;
        buffer_[writePos_] = out;
        writePos_ = (writePos_ + 1) % maxDelay_;
        return out;
    }

    void processBlock(float* buf, int numSamples) {
        for (int i = 0; i < numSamples; ++i)
            buf[i] = processSample(buf[i]);
    }

    void reset() {
        std::fill(buffer_.begin(), buffer_.end(), 0.f);
        writePos_ = 0;
    }

private:
    double             sampleRate_   = 44100.0;
    int                maxDelay_     = 2205;
    int                delaySamples_ = 100;
    float              feedback_     = 0.5f;
    std::vector<float> buffer_;
    int                writePos_     = 0;
};

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

// ─── FormantFilter ───────────────────────────────────────────────────────────
//  3-formant vowel filter — three second-order bandpass filters in parallel.
//  Vowels: 0=A 1=E 2=I 3=O 4=U
class FormantFilter {
public:
    static constexpr int kNumFormants = 3;
    static constexpr int kNumVowels   = 5;

    struct Formant { float freq, bw, gain; };

    static const Formant* vowelFormants(int v) {
        static const Formant data[kNumVowels][kNumFormants] = {
            { {730.f,  80.f, 1.0f}, {1090.f, 120.f, 0.7f}, {2440.f, 180.f, 0.5f} }, // A
            { {270.f,  50.f, 1.0f}, {2290.f, 200.f, 0.7f}, {3010.f, 250.f, 0.4f} }, // E
            { {390.f,  60.f, 1.0f}, {1990.f, 180.f, 0.7f}, {2550.f, 200.f, 0.4f} }, // I
            { {570.f,  70.f, 1.0f}, { 840.f, 100.f, 0.7f}, {2410.f, 200.f, 0.4f} }, // O
            { {300.f,  55.f, 1.0f}, { 870.f, 100.f, 0.7f}, {2240.f, 180.f, 0.4f} }, // U
        };
        return data[juce::jlimit(0, kNumVowels - 1, v)];
    }

    void prepare(double sr) {
        sampleRate_ = sr;
        setVowel(vowel_);
        reset();
    }

    void setVowel(int v) {
        vowel_ = juce::jlimit(0, kNumVowels - 1, v);
        const Formant* f = vowelFormants(vowel_);
        for (int i = 0; i < kNumFormants; ++i)
            computeCoeffs(i, f[i].freq, f[i].bw, f[i].gain);
    }

    float processSample(float x) {
        float out = 0.f;
        for (int i = 0; i < kNumFormants; ++i) {
            float y = b0_[i] * x + b2_[i] * xz2_[i]
                    - a1_[i] * yz1_[i] - a2_[i] * yz2_[i];
            yz2_[i] = yz1_[i]; yz1_[i] = y;
            xz2_[i] = xz1_[i]; xz1_[i] = x;
            out += y * gain_[i];
        }
        return out;
    }

    void processBlock(float* buf, int numSamples) {
        for (int i = 0; i < numSamples; ++i)
            buf[i] = processSample(buf[i]);
    }

    void reset() {
        for (int i = 0; i < kNumFormants; ++i)
            xz1_[i] = xz2_[i] = yz1_[i] = yz2_[i] = 0.f;
    }

private:
    void computeCoeffs(int i, float freq, float bw, float g) {
        gain_[i] = g;
        // Standard biquad bandpass (0 dB peak gain at ω₀)
        // Q = freq / bw
        const double w0    = juce::MathConstants<double>::twoPi * freq / sampleRate_;
        const double alpha = std::sin(w0) * bw / (2.0 * freq);   // sin(w0)/(2Q)
        const double a0inv = 1.0 / (1.0 + alpha);
        b0_[i] = (float)( alpha * a0inv);
        // b1 = 0, b2 = -b0
        b2_[i] = -b0_[i];
        a1_[i] = (float)(-2.0 * std::cos(w0) * a0inv);
        a2_[i] = (float)((1.0 - alpha) * a0inv);
    }

    double sampleRate_ = 44100.0;
    int    vowel_      = 0;

    float b0_[kNumFormants]   = {};
    float b2_[kNumFormants]   = {};
    float a1_[kNumFormants]   = {};
    float a2_[kNumFormants]   = {};
    float gain_[kNumFormants] = {};

    // Biquad delay states
    float xz1_[kNumFormants] = {}, xz2_[kNumFormants] = {};
    float yz1_[kNumFormants] = {}, yz2_[kNumFormants] = {};
};

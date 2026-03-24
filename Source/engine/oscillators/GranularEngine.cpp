#include "GranularEngine.h"
#include <cmath>

void GranularEngine::prepare(double sr) { sampleRate_ = sr; }
void GranularEngine::noteOn()  { active_ = true;  spawnTimer_ = 0.f; }
void GranularEngine::noteOff() { active_ = false; }
void GranularEngine::reset()   { active_ = false; for (auto& g : grains_) g.active = false; }

void GranularEngine::loadSample(const juce::AudioBuffer<float>& buf) {
    sampleBuffer_ = buf;
}

float GranularEngine::hannWindow(float phase) const {
    return 0.5f * (1.f - std::cos(6.28318f * phase));
}

void GranularEngine::spawnGrain() {
    if (sampleBuffer_.getNumSamples() == 0) return;
    for (auto& g : grains_) {
        if (g.active) continue;
        float spread  = spray_ * 0.1f * (float)sampleBuffer_.getNumSamples();
        float basePos = position_ * (float)(sampleBuffer_.getNumSamples() - 1);
        g.position    = juce::jlimit(0.f, (float)(sampleBuffer_.getNumSamples() - 1),
                                     basePos + (rng_.nextFloat() * 2.f - 1.f) * spread);
        g.size        = grainSize_ * 0.001f * (float)sampleRate_;
        g.playhead    = 0.f;
        g.pitchRatio  = std::pow(2.f, (rng_.nextFloat() * 2.f - 1.f) * pitchScat_ / 12.f);
        g.amplitude   = 0.8f + rng_.nextFloat() * 0.2f;
        g.pan         = rng_.nextFloat() * 2.f - 1.f;
        g.active      = true;
        return;
    }
}

void GranularEngine::processBlock(float* outL, float* outR, int numSamples) {
    juce::FloatVectorOperations::clear(outL, numSamples);
    juce::FloatVectorOperations::clear(outR, numSamples);

    if (!active_ || sampleBuffer_.getNumSamples() == 0) return;

    const float spawnInterval = (float)sampleRate_ / std::max(1.f, density_);

    for (int s = 0; s < numSamples; ++s) {
        spawnTimer_ += 1.f;
        if (spawnTimer_ >= spawnInterval) { spawnGrain(); spawnTimer_ = 0.f; }

        for (auto& g : grains_) {
            if (!g.active) continue;

            float window = hannWindow(g.playhead / g.size);
            int   idx0   = (int)(g.position + g.playhead * g.pitchRatio);
            int   idx1   = idx0 + 1;
            int   numSrc = sampleBuffer_.getNumSamples();
            if (idx0 >= numSrc) { g.active = false; continue; }
            idx1 = std::min(idx1, numSrc - 1);

            float alpha  = g.position + g.playhead * g.pitchRatio - (float)idx0;
            const float* src = sampleBuffer_.getReadPointer(0);
            float sample = (src[idx0] + alpha * (src[idx1] - src[idx0])) * window * g.amplitude;

            outL[s] += sample * (1.f - juce::jlimit(0.f, 1.f,  g.pan));
            outR[s] += sample * (1.f - juce::jlimit(0.f, 1.f, -g.pan));

            g.playhead += 1.f;
            if (g.playhead >= g.size) g.active = false;
        }
    }
}

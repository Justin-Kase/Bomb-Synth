#include "WavetableOscillator.h"

void WavetableOscillator::prepare(double sr) {
    sampleRate_ = sr;
    // Trigger library init on audio thread prepare (safe: singleton is const after ctor)
    if (!currentBank_)
        setBankIndex(bankIdx_);
}

void WavetableOscillator::reset() {
    phase_ = 0.0;
}

void WavetableOscillator::setBankIndex(int idx) {
    bankIdx_       = juce::jlimit(0, kWTBanks - 1, idx);
    usingLocalBank_= false;
    currentBank_   = &WavetableBankLibrary::get().bank(bankIdx_);
}

void WavetableOscillator::setFrequency(float hz) {
    frequency_ = hz;
    phaseInc_  = hz / sampleRate_;
}

void WavetableOscillator::setMorphPosition(float pos01) {
    morphPos_ = juce::jlimit(0.f, 1.f, pos01);
}

void WavetableOscillator::loadFrames(const std::vector<std::vector<float>>& frames) {
    localBank_.frames = {};   // zero all frames
    for (int fi = 0; fi < (int)frames.size() && fi < kWTFrames; ++fi) {
        const auto& src = frames[fi];
        for (int i = 0; i < kWTSize && i < (int)src.size(); ++i)
            localBank_.frames[fi].data[i] = src[i];
    }
    usingLocalBank_ = true;
    currentBank_    = &localBank_;
}

float WavetableOscillator::process() {
    if (!currentBank_) return 0.f;
    float s = currentBank_->getSample((float)phase_, morphPos_) * gain_;
    phase_ += phaseInc_;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return s;
}

void WavetableOscillator::processBlock(float* out, int n) {
    if (!currentBank_) {
        std::fill(out, out + n, 0.f);
        return;
    }
    for (int i = 0; i < n; ++i) {
        out[i] = currentBank_->getSample((float)phase_, morphPos_) * gain_;
        phase_ += phaseInc_;
        if (phase_ >= 1.0) phase_ -= 1.0;
    }
}

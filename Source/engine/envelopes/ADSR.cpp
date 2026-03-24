#include "ADSR.h"
#include <cmath>

void ADSR::prepare(double sr) {
    sampleRate_ = sr;
    setParams(params_);
}

void ADSR::setParams(const Params& p) {
    params_ = p;
    auto ms2inc = [&](float ms) {
        return ms > 0.f ? 1.f / (ms * 0.001f * (float)sampleRate_) : 1.f;
    };
    attackInc_  = ms2inc(p.attackMs);
    decayInc_   = ms2inc(p.decayMs);
    releaseInc_ = ms2inc(p.releaseMs);
}

void ADSR::noteOn()  { stage_ = ADSRStage::Attack; }
void ADSR::noteOff() { if (stage_ != ADSRStage::Idle) stage_ = ADSRStage::Release; }
void ADSR::reset()   { stage_ = ADSRStage::Idle; level_ = 0.f; }

float ADSR::applyShape(float t) const {
    if (params_.curve > 0.f)
        return std::pow(t, 1.f + params_.curve * 4.f);   // exponential
    if (params_.curve < 0.f)
        return 1.f - std::pow(1.f - t, 1.f - params_.curve * 4.f); // logarithmic
    return t;
}

float ADSR::process() {
    switch (stage_) {
        case ADSRStage::Idle: return 0.f;

        case ADSRStage::Attack:
            level_ += attackInc_;
            if (level_ >= 1.f) { level_ = 1.f; stage_ = ADSRStage::Decay; }
            return applyShape(level_);

        case ADSRStage::Decay: {
            level_ -= decayInc_;
            if (level_ <= params_.sustain) { level_ = params_.sustain; stage_ = ADSRStage::Sustain; }
            return applyShape(level_);
        }
        case ADSRStage::Sustain:
            return applyShape(params_.sustain);

        case ADSRStage::Release:
            level_ -= releaseInc_;
            if (level_ <= 0.f) { level_ = 0.f; stage_ = ADSRStage::Idle; }
            return applyShape(level_);

        default: return 0.f;
    }
}

void ADSR::processBlock(float* buf, int n) {
    for (int i = 0; i < n; ++i) buf[i] *= process();
}

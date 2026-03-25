#include "Voice.h"
#include <cmath>
#include <algorithm>

void Voice::prepare(double sr, int blockSize) {
    sampleRate_ = sr;
    blockSize_  = blockSize;
    for (auto& o : analogOscs_)    o.prepare(sr);
    for (auto& o : wavetableOscs_) o.prepare(sr);
    for (auto& o : granularOscs_)  o.prepare(sr);
    ladderFilter_.prepare(sr, blockSize);
    svfFilter_.prepare(sr);
    formantFilter_.prepare(sr);
    combFilter_.prepare(sr);
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
        float hz = midiNoteToHz(midiNote, oscTune_[i] + modPitch_);
        analogOscs_[i].setFrequency(hz);
        wavetableOscs_[i].setFrequency(hz);
        if (oscTypes_[i] == OscEngineType::Granular)
            granularOscs_[i].noteOn();
    }
    ampEnv_.noteOn();
    filterEnv_.noteOn();
}

void Voice::noteOff() {
    for (auto& o : granularOscs_) o.noteOff();
    ampEnv_.noteOff();
    filterEnv_.noteOff();
}

void Voice::reset() {
    for (auto& o : analogOscs_)    o.reset();
    for (auto& o : wavetableOscs_) o.reset();
    formantFilter_.reset();
    combFilter_.reset();
    ampEnv_.reset();
    filterEnv_.reset();
}

bool Voice::isActive() const { return !ampEnv_.isIdle(); }

void Voice::setOscEngine(int idx, OscEngineType t) {
    if (idx < 0 || idx >= kNumOscs) return;
    if (oscTypes_[idx] == t) return;
    oscTypes_[idx] = t;
    if (t == OscEngineType::Granular) {
        int bankIdx = (oscBanks_[idx] < 0) ? 0 : oscBanks_[idx];
        const auto& bank = WavetableBankLibrary::get().bank(bankIdx);
        juce::AudioBuffer<float> buf(1, kWTSize);
        std::copy(bank.frames[0].data.data(),
                  bank.frames[0].data.data() + kWTSize,
                  buf.getWritePointer(0));
        granularOscs_[idx].loadSample(buf);
    }
}

void Voice::setOscEngineIdx(int idx, int engineIdx) {
    if (idx < 0 || idx >= kNumOscs) return;
    analogOscs_[idx].resetUnison();   // clear any prior supersaw state

    switch (engineIdx) {
        case OscEngine::WT:
            setOscEngine(idx, OscEngineType::Wavetable); break;
        case OscEngine::GR:
            setOscEngine(idx, OscEngineType::Granular);  break;
        case OscEngine::Sine:
            oscTypes_[idx] = OscEngineType::Analog;
            analogOscs_[idx].setWaveform(OscWaveform::Sine);    break;
        case OscEngine::Saw:
            oscTypes_[idx] = OscEngineType::Analog;
            analogOscs_[idx].setWaveform(OscWaveform::Saw);     break;
        case OscEngine::SuperSaw:
            oscTypes_[idx] = OscEngineType::Analog;
            analogOscs_[idx].setSuperSaw(0.4f);                  break;
        case OscEngine::Square:
            oscTypes_[idx] = OscEngineType::Analog;
            analogOscs_[idx].setWaveform(OscWaveform::Square);  break;
        case OscEngine::Triangle:
            oscTypes_[idx] = OscEngineType::Analog;
            analogOscs_[idx].setWaveform(OscWaveform::Triangle);break;
        case OscEngine::SawTri:
            oscTypes_[idx] = OscEngineType::Analog;
            analogOscs_[idx].setWaveform(OscWaveform::SawTri);  break;
        case OscEngine::Noise:
            oscTypes_[idx] = OscEngineType::Analog;
            analogOscs_[idx].setWaveform(OscWaveform::Noise);   break;
        default: break;
    }
    oscEngineIdx_[idx] = engineIdx;
}

void Voice::setOscBankIndex(int oscIdx, int bankIdx) {
    if (oscIdx < 0 || oscIdx >= kNumOscs) return;
    if (oscBanks_[oscIdx] == bankIdx)     return;
    oscBanks_[oscIdx] = bankIdx;
    wavetableOscs_[oscIdx].setBankIndex(bankIdx);
    if (oscTypes_[oscIdx] == OscEngineType::Granular) {
        const auto& bank = WavetableBankLibrary::get().bank(bankIdx);
        juce::AudioBuffer<float> buf(1, kWTSize);
        std::copy(bank.frames[0].data.data(),
                  bank.frames[0].data.data() + kWTSize,
                  buf.getWritePointer(0));
        granularOscs_[oscIdx].loadSample(buf);
    }
}

void Voice::setOscMorphPos(int oscIdx, float morph01) {
    if (oscIdx >= 0 && oscIdx < kNumOscs) {
        baseMorph_[oscIdx] = morph01;
        // Apply will happen in renderBlock with mod applied
        wavetableOscs_[oscIdx].setMorphPosition(morph01);
    }
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
            float hz = midiNoteToHz(midiNote_, semitones + modPitch_);
            analogOscs_[oscIdx].setFrequency(hz);
            wavetableOscs_[oscIdx].setFrequency(hz);
        }
    }
}

void Voice::renderBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) {
    if (!isActive()) return;

    // Apply per-osc modulation each block
    for (int i = 0; i < kNumOscs; ++i) {
        // Pitch: base tune + global pitch mod + per-osc tune mod + fine mod (cents→semitones)
        float tuneSt = oscTune_[i] + modPitch_ + modTune_[i] + modFine_[i] * 0.01f;
        float hz = midiNoteToHz(midiNote_, tuneSt);
        analogOscs_[i].setFrequency(hz);
        wavetableOscs_[i].setFrequency(hz);
        // Morph
        float morphMod = juce::jlimit(0.f, 1.f, baseMorph_[i] + modMorph_[i]);
        wavetableOscs_[i].setMorphPosition(morphMod);
        // Level
        float levelMod = juce::jlimit(0.f, 1.f, oscLevels_[i] + modLevel_[i]);
        analogOscs_[i].setGain(levelMod);
        wavetableOscs_[i].setGain(levelMod);
        // Morph re-use for analog modes: SuperSaw=detune, Square=PW
        if (oscEngineIdx_[i] == OscEngine::SuperSaw) {
            float detune = juce::jlimit(0.001f, 1.f, baseMorph_[i] + modMorph_[i]);
            analogOscs_[i].setSuperSaw(detune);
        } else if (oscEngineIdx_[i] == OscEngine::Square) {
            float pw = juce::jlimit(0.05f, 0.95f, baseMorph_[i] + modMorph_[i]);
            analogOscs_[i].setPulseWidth(pw);
        }
        // FM
        if (std::abs(modFM_[i]) > 0.001f)
            analogOscs_[i].setFMAmount(juce::jlimit(0.f, 1.f, 0.f + modFM_[i]));
        // Detune: no dedicated setter on WT osc — absorbed via fine pitch offset
        if (std::abs(modDetune_[i]) > 0.001f) {
            // Re-use fine modulation channel with a slight detuning offset
            float detuneHz = hz * (std::pow(2.f, modDetune_[i] * 0.5f / 12.f) - 1.f);
            wavetableOscs_[i].setFrequency(hz + detuneHz);
        }
    }
    // Filter modulation: res + drive
    if (std::abs(modFilterRes_) > 0.001f) {
        float r = juce::jlimit(0.f, 1.f, baseResonance_ + modFilterRes_);
        ladderFilter_.setResonance(r);
        svfFilter_.setResonance(r);
    }
    if (std::abs(modFilterDrive_) > 0.001f) {
        float d = juce::jlimit(1.f, 8.f, 1.f + modFilterDrive_ * 7.f);
        ladderFilter_.setDrive(d);
    }

    // Configure filter based on type
    switch (filterType_) {
        case 0: ladderFilter_.setMode(LadderMode::LP24); break;
        case 1: ladderFilter_.setMode(LadderMode::LP12); break;
        case 2: ladderFilter_.setMode(LadderMode::HP24); break;
        case 3: svfFilter_.setMode(SVFMode::LP);         break;
        case 4: // Formant: resonance controls vowel
            formantFilter_.setVowel(juce::jlimit(0, 4, (int)(baseResonance_ * 4.99f)));
            break;
        case 5: // Comb
            combFilter_.setCutoff(baseCutoff_);
            combFilter_.setResonance(baseResonance_);
            break;
        default: break;
    }

    oscBuf_.setSize(1, numSamples, false, false, true);
    oscBuf_.clear();
    float* mono = oscBuf_.getWritePointer(0);

    float activeLevels = 0.f;
    for (int i = 0; i < kNumOscs; ++i) activeLevels += oscLevels_[i];
    float normGain = (activeLevels > 0.f) ? 1.f / activeLevels : 1.f;

    for (int i = 0; i < kNumOscs; ++i) {
        if (oscLevels_[i] < 1e-4f) continue;

        std::vector<float> tmp(numSamples, 0.f);
        switch (oscTypes_[i]) {
            case OscEngineType::Wavetable:
                wavetableOscs_[i].processBlock(tmp.data(), numSamples);
                break;
            case OscEngineType::Analog:
                analogOscs_[i].processBlock(tmp.data(), numSamples);
                break;
            case OscEngineType::Granular: {
                auto& ge = granularOscs_[i];
                auto& gp = granParams_[i];
                ge.setFrequency(midiNoteToHz(midiNote_, oscTune_[i] + modPitch_));
                ge.setPosition  (juce::jlimit(0.f, 1.f, baseMorph_[i] + modMorph_[i]));
                ge.setDensity   (gp.density);
                ge.setGrainSize (gp.size);
                ge.setSpray     (gp.spray);
                ge.setPitchScatter(gp.pitchScat);
                std::vector<float> tmpR(numSamples, 0.f);
                ge.processBlock(tmp.data(), tmpR.data(), numSamples);
                const float scale = 0.5f * oscLevels_[i];
                for (int s = 0; s < numSamples; ++s)
                    tmp[s] = (tmp[s] + tmpR[s]) * scale;
                break;
            }
            default: break;
        }
        juce::FloatVectorOperations::add(mono, tmp.data(), numSamples);
    }

    juce::FloatVectorOperations::multiply(mono, normGain, numSamples);

    // Filter routing
    switch (filterType_) {
        case 0: case 1: case 2:  // Ladder
            ladderFilter_.processBlock(mono, numSamples);
            break;
        case 3:                  // SVF LP
            svfFilter_.processBlock(mono, numSamples);
            break;
        case 4:                  // Formant
            formantFilter_.processBlock(mono, numSamples);
            break;
        case 5:                  // Comb
            combFilter_.processBlock(mono, numSamples);
            break;
        default:
            ladderFilter_.processBlock(mono, numSamples);
            break;
    }

    // Amp envelope + velocity + gain + mod amp
    const float modAmpMult = juce::jmax(0.f, 1.f + modAmp_);
    for (int s = 0; s < numSamples; ++s)
        mono[s] *= ampEnv_.process() * velocity_ * gain_ * modAmpMult;

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

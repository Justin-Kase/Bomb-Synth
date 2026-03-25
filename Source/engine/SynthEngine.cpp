#include "SynthEngine.h"

void SynthEngine::prepare(double sr, int blockSize) {
    sampleRate_ = sr; blockSize_ = blockSize;
    for (auto& v : voices_) v.prepare(sr, blockSize);
}

void SynthEngine::reset() { for (auto& v : voices_) v.reset(); }

void SynthEngine::setAmpEnvParams(const ADSR::Params& p) {
    ampParams_ = p;
    for (auto& v : voices_) v.setAmpEnvParams(p);
}

void SynthEngine::setFilterEnvParams(const ADSR::Params& p) {
    filterParams_ = p;
    for (auto& v : voices_) v.setFilterEnvParams(p);
}

void SynthEngine::setCutoff(float hz) {
    baseCutoff_ = hz;
    for (auto& v : voices_) v.setCutoff(hz);
}

void SynthEngine::setResonance(float r) {
    for (auto& v : voices_) v.setResonance(r);
}

void SynthEngine::setFilterType(int t) {
    for (auto& v : voices_) v.setFilterType(t);
}

void SynthEngine::setOscEngine(int oscIdx, OscEngineType t) {
    for (auto& v : voices_) v.setOscEngine(oscIdx, t);
}

void SynthEngine::setOscBankIndex(int oscIdx, int bankIdx) {
    for (auto& v : voices_) v.setOscBankIndex(oscIdx, bankIdx);
}

void SynthEngine::setGranularDensity  (int osc, float val) { for (auto& v : voices_) v.setGranularDensity  (osc, val); }
void SynthEngine::setGranularSize     (int osc, float val) { for (auto& v : voices_) v.setGranularSize     (osc, val); }
void SynthEngine::setGranularSpray    (int osc, float val) { for (auto& v : voices_) v.setGranularSpray    (osc, val); }
void SynthEngine::setGranularPitchScat(int osc, float val) { for (auto& v : voices_) v.setGranularPitchScat(osc, val); }
void SynthEngine::setOscWarpMode  (int osc, int  mode)     { for (auto& v : voices_) v.setOscWarpMode  (osc, mode); }
void SynthEngine::setOscWarpAmount(int osc, float amt)      { for (auto& v : voices_) v.setOscWarpAmount(osc, amt);  }

void SynthEngine::setOscMorphPos(int oscIdx, float morph) {
    for (auto& v : voices_) v.setOscMorphPos(oscIdx, morph);
}

void SynthEngine::setOscLevel(int oscIdx, float level) {
    for (auto& v : voices_) v.setOscLevel(oscIdx, level);
}

void SynthEngine::setOscTune(int oscIdx, float semitones) {
    for (auto& v : voices_) v.setOscTune(oscIdx, semitones);
}

Voice* SynthEngine::findFreeVoice(int) {
    for (auto& v : voices_) if (!v.isActive()) return &v;
    return nullptr;
}

Voice* SynthEngine::stealVoice(int) { return &voices_[0]; }

void SynthEngine::handleNoteOn(int note, float vel) {
    for (auto& v : voices_)
        if (v.isActive() && v.getMidiNote() == note) v.noteOff();
    Voice* v = findFreeVoice(note);
    if (!v) v = stealVoice(note);
    v->reset();
    v->setCutoff(baseCutoff_);
    v->setResonance(0.f);
    v->setAmpEnvParams(ampParams_);
    v->setFilterEnvParams(filterParams_);
    v->noteOn(note, vel);
}

void SynthEngine::handleNoteOff(int note) {
    for (auto& v : voices_)
        if (v.isActive() && v.getMidiNote() == note) v.noteOff();
}

void SynthEngine::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi) {
    audio.clear();

    // Apply modulation offsets to all active voices before rendering
    const float effectiveCutoff = juce::jmax(20.f, baseCutoff_ + modCutoffHz_);
    for (auto& v : voices_) {
        if (!v.isActive()) continue;
        v.setCutoff(effectiveCutoff);
        v.setModPitch     (modPitchSemitones_);
        v.setModAmp       (modAmp_);
        v.setModFilterRes (modFilterRes_);
        v.setModFilterDrive(modFilterDrive_);
        for (int i = 0; i < 3; ++i) {
            v.setModMorph  (i, modMorph_[i]);
            v.setModTune   (i, modTune_[i]);
            v.setModFine   (i, modFine_[i]);
            v.setModLevel  (i, modLevel_[i]);
            v.setModFM     (i, modFM_[i]);
            v.setModDetune (i, modDetune_[i]);
        }
    }

    int numSamples = audio.getNumSamples(), pos = 0;

    for (auto meta : midi) {
        auto msg    = meta.getMessage();
        int  offset = meta.samplePosition;
        if (offset > pos) {
            for (auto& v : voices_) v.renderBlock(audio, pos, offset - pos);
            pos = offset;
        }
        if (msg.isNoteOn())  handleNoteOn (msg.getNoteNumber(), msg.getVelocity() / 127.f);
        if (msg.isNoteOff()) handleNoteOff(msg.getNoteNumber());
    }
    if (pos < numSamples)
        for (auto& v : voices_) v.renderBlock(audio, pos, numSamples - pos);

    audio.applyGain(masterGain_);
}

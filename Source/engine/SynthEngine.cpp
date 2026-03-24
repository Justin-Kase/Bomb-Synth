#include "SynthEngine.h"

void SynthEngine::prepare(double sr, int blockSize) {
    sampleRate_ = sr;
    blockSize_  = blockSize;
    for (auto& v : voices_) v.prepare(sr, blockSize);
}

void SynthEngine::reset() {
    for (auto& v : voices_) v.reset();
}

void SynthEngine::setAmpEnvParams(const ADSR::Params& p) {
    ampParams_ = p;
    for (auto& v : voices_) v.setAmpEnvParams(p);
}

void SynthEngine::setFilterEnvParams(const ADSR::Params& p) {
    filterParams_ = p;
    for (auto& v : voices_) v.setFilterEnvParams(p);
}

void SynthEngine::setCutoff(float hz) {
    cutoff_ = hz;
    for (auto& v : voices_) v.setCutoff(hz);
}

void SynthEngine::setResonance(float r) {
    resonance_ = r;
    for (auto& v : voices_) v.setResonance(r);
}

Voice* SynthEngine::findFreeVoice(int /*note*/) {
    for (auto& v : voices_)
        if (!v.isActive()) return &v;
    return nullptr;
}

Voice* SynthEngine::stealVoice(int /*note*/) {
    // Steal oldest active voice (first in array for now)
    return &voices_[0];
}

void SynthEngine::handleNoteOn(int note, float vel) {
    // Same note — steal its own voice first
    for (auto& v : voices_)
        if (v.isActive() && v.getMidiNote() == note) { v.noteOff(); }

    Voice* v = findFreeVoice(note);
    if (!v) v = stealVoice(note);
    v->reset();
    v->setCutoff(cutoff_);
    v->setResonance(resonance_);
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
    int numSamples = audio.getNumSamples();
    int pos        = 0;

    for (auto meta : midi) {
        auto msg    = meta.getMessage();
        int  offset = meta.samplePosition;

        // Render up to this MIDI event
        if (offset > pos) {
            for (auto& v : voices_)
                v.renderBlock(audio, pos, offset - pos);
            pos = offset;
        }

        if (msg.isNoteOn())
            handleNoteOn(msg.getNoteNumber(), msg.getVelocity() / 127.f);
        else if (msg.isNoteOff())
            handleNoteOff(msg.getNoteNumber());
    }

    // Render remainder
    if (pos < numSamples)
        for (auto& v : voices_)
            v.renderBlock(audio, pos, numSamples - pos);

    // Master gain
    audio.applyGain(masterGain_);
}

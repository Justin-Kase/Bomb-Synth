#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout BombSynthAudioProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // ── Master ───────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<juce::AudioParameterFloat>("master_gain", "Master Volume",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.8f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("glide_time",  "Glide Time",
        juce::NormalisableRange<float>{0.f, 5000.f, 0.f, 0.3f}, 0.f));

    // ── Oscillators ──────────────────────────────────────────────────────────
    for (int i = 1; i <= 3; ++i) {
        juce::String n(i);
        // bank index: 0-5 (replaces old 0-4 wave selector)
        p.push_back(std::make_unique<juce::AudioParameterInt>  ("osc"+n+"_wave",  "OSC "+n+" Bank",  0, 31, 0));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("osc"+n+"_morph", "OSC "+n+" Morph",
            juce::NormalisableRange<float>{0.f, 1.f}, 0.f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("osc"+n+"_tune",  "OSC "+n+" Tune",
            juce::NormalisableRange<float>{-24.f, 24.f, 1.f}, 0.f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("osc"+n+"_fine",  "OSC "+n+" Fine",
            juce::NormalisableRange<float>{-100.f, 100.f}, 0.f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("osc"+n+"_level", "OSC "+n+" Level",
            juce::NormalisableRange<float>{0.f, 1.f}, i == 1 ? 1.f : 0.f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("osc"+n+"_fm",    "OSC "+n+" FM",
            juce::NormalisableRange<float>{0.f, 1.f}, 0.f));
        p.push_back(std::make_unique<juce::AudioParameterInt>  ("osc"+n+"_uni",   "OSC "+n+" Unison", 1, 8, 1));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("osc"+n+"_detune","OSC "+n+" Detune",
            juce::NormalisableRange<float>{0.f, 1.f}, 0.f));
    }

    // ── Filter ───────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<juce::AudioParameterFloat>("filter_cutoff", "Cutoff",
        juce::NormalisableRange<float>{20.f, 20000.f, 0.f, 0.25f}, 6000.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("filter_res",    "Resonance",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("filter_drive",  "Drive",
        juce::NormalisableRange<float>{1.f, 8.f, 0.f, 0.5f}, 1.f));
    p.push_back(std::make_unique<juce::AudioParameterInt>  ("filter_type",   "Filter Type", 0, 4, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("filter_env_amt","Filter Env Amt",
        juce::NormalisableRange<float>{-1.f, 1.f}, 0.f));

    // ── Amp Envelope ─────────────────────────────────────────────────────────
    p.push_back(std::make_unique<juce::AudioParameterFloat>("amp_attack",  "Amp Attack",
        juce::NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 5.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("amp_decay",   "Amp Decay",
        juce::NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 150.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("amp_sustain", "Amp Sustain",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.75f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("amp_release", "Amp Release",
        juce::NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 300.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("amp_curve",   "Amp Curve",
        juce::NormalisableRange<float>{-1.f, 1.f}, 0.f));

    // ── Filter Envelope ───────────────────────────────────────────────────────
    p.push_back(std::make_unique<juce::AudioParameterFloat>("fenv_attack",  "Filter Attack",
        juce::NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 5.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("fenv_decay",   "Filter Decay",
        juce::NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 300.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("fenv_sustain", "Filter Sustain",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("fenv_release", "Filter Release",
        juce::NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 500.f));

    // ── LFO 1 ────────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<juce::AudioParameterInt>  ("lfo1_shape", "LFO1 Shape", 0, 6, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("lfo1_rate",  "LFO1 Rate",
        juce::NormalisableRange<float>{0.01f, 30.f, 0.f, 0.4f}, 1.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("lfo1_depth", "LFO1 Depth",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("lfo1_phase", "LFO1 Phase",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.f));

    // ── LFO 2 ────────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<juce::AudioParameterInt>  ("lfo2_shape", "LFO2 Shape", 0, 6, 2));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("lfo2_rate",  "LFO2 Rate",
        juce::NormalisableRange<float>{0.01f, 30.f, 0.f, 0.4f}, 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("lfo2_depth", "LFO2 Depth",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.f));

    // ── Reverb ────────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<juce::AudioParameterFloat>("reverb_room",  "Reverb Room",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("reverb_damp",  "Reverb Damp",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("reverb_width", "Reverb Width",
        juce::NormalisableRange<float>{0.f, 1.f}, 1.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("reverb_wet",   "Reverb Wet",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.f));

    // ── Delay ─────────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<juce::AudioParameterFloat>("delay_time", "Delay Time",
        juce::NormalisableRange<float>{1.f, 2000.f, 0.f, 0.4f}, 250.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("delay_fb",   "Delay Feedback",
        juce::NormalisableRange<float>{0.f, 0.95f}, 0.4f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("delay_wet",  "Delay Wet",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.f));

    // ── Chorus ────────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<juce::AudioParameterFloat>("chorus_rate",  "Chorus Rate",
        juce::NormalisableRange<float>{0.1f, 8.f, 0.f, 0.5f}, 1.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("chorus_depth", "Chorus Depth",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("chorus_wet",   "Chorus Wet",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.f));

    return { p.begin(), p.end() };
}

BombSynthAudioProcessor::BombSynthAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Force wavetable library initialisation on the UI thread at load time
    (void)WavetableBankLibrary::get();
}

void BombSynthAudioProcessor::prepareToPlay(double sr, int blockSize) {
    sampleRate_ = sr;
    engine_.prepare(sr, blockSize);

    // Reverb
    {
        juce::dsp::ProcessSpec revSpec;
        revSpec.sampleRate       = sr;
        revSpec.maximumBlockSize = (juce::uint32)blockSize;
        revSpec.numChannels      = 2;
        reverb_.prepare(revSpec);
        reverb_.reset();
    }

    // Chorus
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = (juce::uint32)blockSize;
    spec.numChannels      = 2;
    chorus_.prepare(spec);

    // Delay ring buffers (2s max)
    const int maxSmp = (int)(sr * 2.0) + 1;
    for (auto& buf : delayBuf_) { buf.assign(maxSmp, 0.f); }
    delayWrite_ = 0;
}
void BombSynthAudioProcessor::releaseResources() { engine_.reset(); }

#ifndef JucePlugin_PreferredChannelConfigurations
bool BombSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
#endif

void BombSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals nd;

    auto get  = [&](const char* id)  { return params_.getRawParameterValue(id)->load(); };
    auto geti = [&](const char* id)  { return (int)params_.getRawParameterValue(id)->load(); };

    // ── Oscillators ──────────────────────────────────────────────────────────
    static const char* bankIds[3]   = {"osc1_wave","osc2_wave","osc3_wave"};
    static const char* morphIds[3]  = {"osc1_morph","osc2_morph","osc3_morph"};
    static const char* levelIds[3]  = {"osc1_level","osc2_level","osc3_level"};
    static const char* tuneIds[3]   = {"osc1_tune","osc2_tune","osc3_tune"};

    for (int i = 0; i < 3; ++i) {
        engine_.setOscBankIndex(i, geti(bankIds[i]));
        engine_.setOscMorphPos (i, get (morphIds[i]));
        engine_.setOscLevel    (i, get (levelIds[i]));
        engine_.setOscTune     (i, get (tuneIds[i]));
    }

    // ── Filter ───────────────────────────────────────────────────────────────
    engine_.setCutoff   (get("filter_cutoff"));
    engine_.setResonance(get("filter_res"));

    // ── Envelopes ─────────────────────────────────────────────────────────────
    ADSR::Params amp;
    amp.attackMs  = get("amp_attack");
    amp.decayMs   = get("amp_decay");
    amp.sustain   = get("amp_sustain");
    amp.releaseMs = get("amp_release");
    amp.curve     = get("amp_curve");

    ADSR::Params fenv;
    fenv.attackMs  = get("fenv_attack");
    fenv.decayMs   = get("fenv_decay");
    fenv.sustain   = get("fenv_sustain");
    fenv.releaseMs = get("fenv_release");

    engine_.setAmpEnvParams   (amp);
    engine_.setFilterEnvParams(fenv);
    engine_.setMasterGain(get("master_gain"));
    engine_.processBlock(buf, midi);

    const int numSamples = buf.getNumSamples();
    if (buf.getNumChannels() < 2) return;

    float* L = buf.getWritePointer(0);
    float* R = buf.getWritePointer(1);

    // ── Delay ─────────────────────────────────────────────────────────────────
    {
        const float delayWet = get("delay_wet");
        if (delayWet > 0.001f) {
            const float fb   = get("delay_fb");
            const int   dSmp = juce::jlimit(1, (int)delayBuf_[0].size() - 1,
                                             (int)(get("delay_time") * 0.001f * sampleRate_));
            const int   bufSz = (int)delayBuf_[0].size();

            for (int i = 0; i < numSamples; ++i) {
                int rPos = (delayWrite_ - dSmp + bufSz) % bufSz;
                float dL = delayBuf_[0][rPos];
                float dR = delayBuf_[1][rPos];
                delayBuf_[0][delayWrite_] = L[i] + dL * fb;
                delayBuf_[1][delayWrite_] = R[i] + dR * fb;
                L[i] = L[i] * (1.f - delayWet) + dL * delayWet;
                R[i] = R[i] * (1.f - delayWet) + dR * delayWet;
                delayWrite_ = (delayWrite_ + 1) % bufSz;
            }
        }
    }

    // ── Chorus ────────────────────────────────────────────────────────────────
    {
        const float choWet = get("chorus_wet");
        if (choWet > 0.001f) {
            chorus_.setRate       (get("chorus_rate"));
            chorus_.setDepth      (get("chorus_depth"));
            chorus_.setCentreDelay(7.f);
            chorus_.setFeedback   (0.f);
            chorus_.setMix        (choWet);

            juce::dsp::AudioBlock<float>            block(buf);
            juce::dsp::ProcessContextReplacing<float> ctx(block);
            chorus_.process(ctx);
        }
    }

    // ── Reverb ────────────────────────────────────────────────────────────────
    {
        const float revWet = get("reverb_wet");
        if (revWet > 0.001f) {
            juce::dsp::Reverb::Parameters rp;
            rp.roomSize   = get("reverb_room");
            rp.damping    = get("reverb_damp");
            rp.width      = get("reverb_width");
            rp.wetLevel   = revWet;
            rp.dryLevel   = 1.f - revWet * 0.5f;
            rp.freezeMode = 0.f;
            reverb_.setParameters(rp);
            juce::dsp::AudioBlock<float>             revBlock(buf);
            juce::dsp::ProcessContextReplacing<float> revCtx(revBlock);
            reverb_.process(revCtx);
        }
    }
}

juce::AudioProcessorEditor* BombSynthAudioProcessor::createEditor() {
    return new BombSynthAudioProcessorEditor(*this);
}

void BombSynthAudioProcessor::getStateInformation(juce::MemoryBlock& d) {
    auto s = params_.copyState();
    if (auto x = s.createXml()) copyXmlToBinary(*x, d);
}
void BombSynthAudioProcessor::setStateInformation(const void* d, int sz) {
    if (auto x = std::unique_ptr<juce::XmlElement>(getXmlFromBinary(d, sz)))
        params_.replaceState(juce::ValueTree::fromXml(*x));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BombSynthAudioProcessor(); }

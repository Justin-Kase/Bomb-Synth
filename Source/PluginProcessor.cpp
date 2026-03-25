#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "engine/oscillators/WavetableBank.h"
#include <juce_audio_formats/juce_audio_formats.h>

juce::AudioProcessorValueTreeState::ParameterLayout BombSynthAudioProcessor::createParameters() {
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    // ── Master ───────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<AudioParameterFloat>("master_gain", "Master Volume",
        NormalisableRange<float>{0.f, 1.f}, 0.8f));
    p.push_back(std::make_unique<AudioParameterFloat>("glide_time",  "Glide Time",
        NormalisableRange<float>{0.f, 5000.f, 0.f, 0.3f}, 0.f));

    // ── Oscillators ──────────────────────────────────────────────────────────
    for (int i = 1; i <= 3; ++i) {
        String n(i);
        p.push_back(std::make_unique<AudioParameterInt>  ("osc"+n+"_wave",  "OSC "+n+" Bank",  0, 31, 0));
        p.push_back(std::make_unique<AudioParameterFloat>("osc"+n+"_morph", "OSC "+n+" Morph",
            NormalisableRange<float>{0.f, 1.f}, 0.f));
        p.push_back(std::make_unique<juce::AudioParameterInt>  ("osc"+n+"_oct",   "OSC "+n+" Octave", -3, 3, 0));
        p.push_back(std::make_unique<AudioParameterFloat>("osc"+n+"_tune",  "OSC "+n+" Tune",
            NormalisableRange<float>{-24.f, 24.f, 1.f}, 0.f));
        p.push_back(std::make_unique<AudioParameterFloat>("osc"+n+"_fine",  "OSC "+n+" Fine",
            NormalisableRange<float>{-100.f, 100.f}, 0.f));
        p.push_back(std::make_unique<AudioParameterFloat>("osc"+n+"_level", "OSC "+n+" Level",
            NormalisableRange<float>{0.f, 1.f}, i == 1 ? 1.f : 0.f));
        p.push_back(std::make_unique<AudioParameterFloat>("osc"+n+"_fm",    "OSC "+n+" FM",
            NormalisableRange<float>{0.f, 1.f}, 0.f));
        p.push_back(std::make_unique<AudioParameterInt>  ("osc"+n+"_uni",   "OSC "+n+" Unison", 1, 8, 1));
        p.push_back(std::make_unique<AudioParameterFloat>("osc"+n+"_detune","OSC "+n+" Detune",
            NormalisableRange<float>{0.f, 1.f}, 0.f));
        // Engine type (0=Wavetable, 1=Granular)
        p.push_back(std::make_unique<AudioParameterInt>  ("osc"+n+"_engine", "OSC"+n+" Engine", 0, 1, 0));
        // Granular
        p.push_back(std::make_unique<AudioParameterFloat>("osc"+n+"_gran_density", "OSC"+n+" Density",
            NormalisableRange<float>{1.f, 100.f, 0.f, 0.5f}, 20.f));
        p.push_back(std::make_unique<AudioParameterFloat>("osc"+n+"_gran_size", "OSC"+n+" Grain Size",
            NormalisableRange<float>{5.f, 500.f, 0.f, 0.5f}, 80.f));
        p.push_back(std::make_unique<AudioParameterFloat>("osc"+n+"_gran_spray", "OSC"+n+" Spray",
            NormalisableRange<float>{0.f, 1.f}, 0.1f));
        p.push_back(std::make_unique<AudioParameterFloat>("osc"+n+"_gran_pitch", "OSC"+n+" Pitch Scatter",
            NormalisableRange<float>{0.f, 24.f}, 0.f));
        // Warp
        p.push_back(std::make_unique<AudioParameterInt>  ("osc"+n+"_warp_mode", "OSC"+n+" Warp Mode", 0, 3, 0));
        p.push_back(std::make_unique<AudioParameterFloat>("osc"+n+"_warp_amt",  "OSC"+n+" Warp Amount",
            NormalisableRange<float>{0.f, 1.f}, 0.f));
    }

    // ── Filter ───────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<AudioParameterFloat>("filter_cutoff", "Cutoff",
        NormalisableRange<float>{20.f, 20000.f, 0.f, 0.25f}, 6000.f));
    p.push_back(std::make_unique<AudioParameterFloat>("filter_res",    "Resonance",
        NormalisableRange<float>{0.f, 1.f}, 0.f));
    p.push_back(std::make_unique<AudioParameterFloat>("filter_drive",  "Drive",
        NormalisableRange<float>{1.f, 8.f, 0.f, 0.5f}, 1.f));
    p.push_back(std::make_unique<AudioParameterInt>  ("filter_type",   "Filter Type", 0, 5, 0));
    p.push_back(std::make_unique<AudioParameterFloat>("filter_env_amt","Filter Env Amt",
        NormalisableRange<float>{-1.f, 1.f}, 0.f));

    // ── Amp Envelope ─────────────────────────────────────────────────────────
    p.push_back(std::make_unique<AudioParameterFloat>("amp_attack",  "Amp Attack",
        NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 5.f));
    p.push_back(std::make_unique<AudioParameterFloat>("amp_decay",   "Amp Decay",
        NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 150.f));
    p.push_back(std::make_unique<AudioParameterFloat>("amp_sustain", "Amp Sustain",
        NormalisableRange<float>{0.f, 1.f}, 0.75f));
    p.push_back(std::make_unique<AudioParameterFloat>("amp_release", "Amp Release",
        NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 300.f));
    p.push_back(std::make_unique<AudioParameterFloat>("amp_curve",   "Amp Curve",
        NormalisableRange<float>{-1.f, 1.f}, 0.f));

    // ── Filter Envelope ───────────────────────────────────────────────────────
    p.push_back(std::make_unique<AudioParameterFloat>("fenv_attack",  "Filter Attack",
        NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 5.f));
    p.push_back(std::make_unique<AudioParameterFloat>("fenv_decay",   "Filter Decay",
        NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 300.f));
    p.push_back(std::make_unique<AudioParameterFloat>("fenv_sustain", "Filter Sustain",
        NormalisableRange<float>{0.f, 1.f}, 0.5f));
    p.push_back(std::make_unique<AudioParameterFloat>("fenv_release", "Filter Release",
        NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 500.f));

    // ── LFO 1 ────────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<AudioParameterInt>  ("lfo1_shape", "LFO1 Shape", 0, 6, 0));
    p.push_back(std::make_unique<AudioParameterFloat>("lfo1_rate",  "LFO1 Rate",
        NormalisableRange<float>{0.01f, 30.f, 0.f, 0.4f}, 1.f));
    p.push_back(std::make_unique<AudioParameterFloat>("lfo1_depth", "LFO1 Depth",
        NormalisableRange<float>{0.f, 1.f}, 0.f));
    p.push_back(std::make_unique<AudioParameterFloat>("lfo1_phase", "LFO1 Phase",
        NormalisableRange<float>{0.f, 1.f}, 0.f));

    // ── LFO 2 ────────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<AudioParameterInt>  ("lfo2_shape", "LFO2 Shape", 0, 6, 2));
    p.push_back(std::make_unique<AudioParameterFloat>("lfo2_rate",  "LFO2 Rate",
        NormalisableRange<float>{0.01f, 30.f, 0.f, 0.4f}, 0.5f));
    p.push_back(std::make_unique<AudioParameterFloat>("lfo2_depth", "LFO2 Depth",
        NormalisableRange<float>{0.f, 1.f}, 0.f));

    // ── Mod Routing Slots (8×) ───────────────────────────────────────────────
    // Source: 0=Off 1=LFO1 2=LFO2 3=Velocity 4=ModWheel
    // Target: 0=Off 1=Cutoff 2=Res 3=Drive 4=Pitch 5=Amp
    //         6=O1Morph 7=O2Morph 8=O3Morph
    //         9=O1Tune 10=O2Tune 11=O3Tune
    //         12=O1Fine 13=O2Fine 14=O3Fine
    //         15=O1Level 16=O2Level 17=O3Level
    //         18=O1FM 19=O2FM 20=O3FM
    //         21=O1Detune 22=O2Detune 23=O3Detune
    //         24=LFO2Rate
    for (int i = 0; i < 8; ++i) {
        String s(i);
        p.push_back(std::make_unique<AudioParameterInt>  ("mod"+s+"_src", "Mod"+s+" Source", 0, 4, 0));
        p.push_back(std::make_unique<AudioParameterInt>  ("mod"+s+"_dst", "Mod"+s+" Target", 0, 24, 0));
        p.push_back(std::make_unique<AudioParameterFloat>("mod"+s+"_amt", "Mod"+s+" Amount",
            NormalisableRange<float>{-1.f, 1.f}, i == 0 ? 0.5f : 0.f));
    }

    // ── Reverb ────────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<AudioParameterFloat>("reverb_room",  "Reverb Room",
        NormalisableRange<float>{0.f, 1.f}, 0.5f));
    p.push_back(std::make_unique<AudioParameterFloat>("reverb_damp",  "Reverb Damp",
        NormalisableRange<float>{0.f, 1.f}, 0.5f));
    p.push_back(std::make_unique<AudioParameterFloat>("reverb_width", "Reverb Width",
        NormalisableRange<float>{0.f, 1.f}, 1.0f));
    p.push_back(std::make_unique<AudioParameterFloat>("reverb_wet",   "Reverb Wet",
        NormalisableRange<float>{0.f, 1.f}, 0.f));

    // ── Delay ─────────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<AudioParameterFloat>("delay_time", "Delay Time",
        NormalisableRange<float>{1.f, 2000.f, 0.f, 0.4f}, 250.f));
    p.push_back(std::make_unique<AudioParameterFloat>("delay_fb",   "Delay Feedback",
        NormalisableRange<float>{0.f, 0.95f}, 0.4f));
    p.push_back(std::make_unique<AudioParameterFloat>("delay_wet",  "Delay Wet",
        NormalisableRange<float>{0.f, 1.f}, 0.f));

    // ── Chorus ────────────────────────────────────────────────────────────────
    p.push_back(std::make_unique<AudioParameterFloat>("chorus_rate",  "Chorus Rate",
        NormalisableRange<float>{0.1f, 8.f, 0.f, 0.5f}, 1.5f));
    p.push_back(std::make_unique<AudioParameterFloat>("chorus_depth", "Chorus Depth",
        NormalisableRange<float>{0.f, 1.f}, 0.5f));
    p.push_back(std::make_unique<AudioParameterFloat>("chorus_wet",   "Chorus Wet",
        NormalisableRange<float>{0.f, 1.f}, 0.f));

    return { p.begin(), p.end() };
}

BombSynthAudioProcessor::BombSynthAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    (void)WavetableBankLibrary::get();
}

void BombSynthAudioProcessor::prepareToPlay(double sr, int blockSize) {
    sampleRate_ = sr;
    sequencer_.prepare(sr);
    engine_.prepare(sr, blockSize);

    lfo1_.prepare(sr, blockSize);
    lfo2_.prepare(sr, blockSize);

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

void BombSynthAudioProcessor::addUserWavetablePath(const juce::String& path) {
    if (!userWavetablePaths_.contains(path))
        userWavetablePaths_.add(path);
}

void BombSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals nd;

    auto get  = [&](const char* id) { return params_.getRawParameterValue(id)->load(); };
    auto geti = [&](const char* id) { return (int)params_.getRawParameterValue(id)->load(); };

    // ── Oscillators ──────────────────────────────────────────────────────────
    static const char* bankIds[3]  = {"osc1_wave","osc2_wave","osc3_wave"};
    static const char* morphIds[3] = {"osc1_morph","osc2_morph","osc3_morph"};
    static const char* levelIds[3] = {"osc1_level","osc2_level","osc3_level"};
    static const char* tuneIds[3]  = {"osc1_tune","osc2_tune","osc3_tune"};

    static const char* engineIds[3]   = {"osc1_engine",     "osc2_engine",     "osc3_engine"};
    static const char* granDensIds[3] = {"osc1_gran_density","osc2_gran_density","osc3_gran_density"};
    static const char* granSizeIds[3] = {"osc1_gran_size",   "osc2_gran_size",   "osc3_gran_size"};
    static const char* granSprayIds[3]= {"osc1_gran_spray",  "osc2_gran_spray",  "osc3_gran_spray"};
    static const char* granPitchIds[3]= {"osc1_gran_pitch",  "osc2_gran_pitch",  "osc3_gran_pitch"};
    static const char* warpModeIds[3] = {"osc1_warp_mode",   "osc2_warp_mode",   "osc3_warp_mode"};
    static const char* warpAmtIds[3]  = {"osc1_warp_amt",    "osc2_warp_amt",    "osc3_warp_amt"};

    for (int i = 0; i < 3; ++i) {
        engine_.setOscBankIndex(i, geti(bankIds[i]));
        engine_.setOscMorphPos (i, get (morphIds[i]));
        engine_.setOscLevel    (i, get (levelIds[i]));
        static const char* octIds[3]   = {"osc1_oct","osc2_oct","osc3_oct"};
        engine_.setOscTune     (i, get(tuneIds[i]) + (float)geti(octIds[i]) * 12.f);
        auto eng = (geti(engineIds[i]) == 0) ? OscEngineType::Wavetable : OscEngineType::Granular;
        engine_.setOscEngine        (i, eng);
        engine_.setGranularDensity  (i, get (granDensIds[i]));
        engine_.setGranularSize     (i, get (granSizeIds[i]));
        engine_.setGranularSpray    (i, get (granSprayIds[i]));
        engine_.setGranularPitchScat(i, get (granPitchIds[i]));
        engine_.setOscWarpMode      (i, geti(warpModeIds[i]));
        engine_.setOscWarpAmount    (i, get (warpAmtIds[i]));
    }

    // ── Filter ───────────────────────────────────────────────────────────────
    engine_.setCutoff   (get("filter_cutoff"));
    engine_.setResonance(get("filter_res"));
    engine_.setFilterType(geti("filter_type"));

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
    // Capture ModWheel (CC1) from incoming MIDI
    for (const auto meta : midi) {
        const auto msg = meta.getMessage();
        if (msg.isController() && msg.getControllerNumber() == 1)
            modWheelValue_ = msg.getControllerValue() / 127.f;
    }

    engine_.setMasterGain(get("master_gain"));

    // ── LFO processing ────────────────────────────────────────────────────────
    lfo1_.setShape(geti("lfo1_shape"));
    lfo1_.setRate (get ("lfo1_rate"));
    lfo1_.setDepth(get ("lfo1_depth"));
    lfo1_.setPhase(get ("lfo1_phase"));

    // Allow LFO2 rate to be modulated (by mod slot targeting LFO2Rate)
    float lfo2Rate = get("lfo2_rate");
    lfo2_.setShape(geti("lfo2_shape"));
    lfo2_.setRate (lfo2Rate);
    lfo2_.setDepth(get("lfo2_depth"));

    const float lfo1val = lfo1_.tick();
    const float lfo2val = lfo2_.tick();

    // ── Mod routing slots ──────────────────────────────────────────────────────
    modCutoff_      = 0.f;
    modPitch_       = 0.f;
    modAmp_         = 0.f;
    modFilterRes_   = 0.f;
    modFilterDrive_ = 0.f;
    modMorph_   = { 0.f, 0.f, 0.f };
    modTune_    = { 0.f, 0.f, 0.f };
    modFine_    = { 0.f, 0.f, 0.f };
    modLevel_   = { 0.f, 0.f, 0.f };
    modFM_      = { 0.f, 0.f, 0.f };
    modDetune_  = { 0.f, 0.f, 0.f };

    // ModWheel: read current MIDI CC1 value (0–1)
    float modWheelVal = modWheelValue_;

    for (int i = 0; i < 8; ++i) {
        juce::String s(i);
        const int   src = geti(("mod" + s + "_src").toRawUTF8());
        const int   dst = geti(("mod" + s + "_dst").toRawUTF8());
        const float amt = get (("mod" + s + "_amt").toRawUTF8());

        if (src == 0 || dst == 0) continue;

        float modVal = 0.f;
        switch (src) {
            case 1: modVal = lfo1val;      break;   // LFO1
            case 2: modVal = lfo2val;      break;   // LFO2
            case 3: modVal = 0.f;          break;   // Velocity — per-note, not block-rate
            case 4: modVal = modWheelVal;  break;   // ModWheel
            default: break;
        }
        modVal *= amt;

        switch (dst) {
            // Global
            case  1: modCutoff_      += modVal * 10000.f; break;  // Filter Cutoff (Hz)
            case  2: modFilterRes_   += modVal;            break;  // Filter Res (0-1 delta)
            case  3: modFilterDrive_ += modVal;            break;  // Filter Drive (0-1 → 0-7x)
            case  4: modPitch_       += modVal * 12.f;     break;  // Global Pitch ±12st
            case  5: modAmp_         += modVal;            break;  // Amp
            // Per-osc Morph
            case  6: modMorph_[0]   += modVal; break;
            case  7: modMorph_[1]   += modVal; break;
            case  8: modMorph_[2]   += modVal; break;
            // Per-osc Tune (semitones)
            case  9: modTune_[0]    += modVal * 12.f; break;
            case 10: modTune_[1]    += modVal * 12.f; break;
            case 11: modTune_[2]    += modVal * 12.f; break;
            // Per-osc Fine (cents)
            case 12: modFine_[0]    += modVal * 100.f; break;
            case 13: modFine_[1]    += modVal * 100.f; break;
            case 14: modFine_[2]    += modVal * 100.f; break;
            // Per-osc Level
            case 15: modLevel_[0]   += modVal; break;
            case 16: modLevel_[1]   += modVal; break;
            case 17: modLevel_[2]   += modVal; break;
            // Per-osc FM
            case 18: modFM_[0]      += modVal; break;
            case 19: modFM_[1]      += modVal; break;
            case 20: modFM_[2]      += modVal; break;
            // Per-osc Detune
            case 21: modDetune_[0]  += modVal; break;
            case 22: modDetune_[1]  += modVal; break;
            case 23: modDetune_[2]  += modVal; break;
            // LFO2 Rate
            case 24: lfo2_.setRate(juce::jmax(0.01f, lfo2Rate + modVal * 10.f)); break;
            default: break;
        }
    }

    engine_.setModCutoffHz      (modCutoff_);
    engine_.setModPitchSemitones(modPitch_);
    engine_.setModAmp           (modAmp_);
    engine_.setModFilterRes     (modFilterRes_);
    engine_.setModFilterDrive   (modFilterDrive_);
    for (int i = 0; i < 3; ++i) {
        engine_.setModMorph  (i, modMorph_[i]);
        engine_.setModTune   (i, modTune_[i]);
        engine_.setModFine   (i, modFine_[i]);
        engine_.setModLevel  (i, modLevel_[i]);
        engine_.setModFM     (i, modFM_[i]);
        engine_.setModDetune (i, modDetune_[i]);
    }

    // Inject sequencer MIDI before engine processes
    sequencer_.processBlock(midi, buf.getNumSamples(), getPlayHead());

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

            juce::dsp::AudioBlock<float>              block(buf);
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
            juce::dsp::AudioBlock<float>              revBlock(buf);
            juce::dsp::ProcessContextReplacing<float> revCtx(revBlock);
            reverb_.process(revCtx);
        }
    }
}

juce::AudioProcessorEditor* BombSynthAudioProcessor::createEditor() {
    return new BombSynthAudioProcessorEditor(*this);
}

void BombSynthAudioProcessor::getStateInformation(juce::MemoryBlock& d) {
    auto root = std::make_unique<juce::XmlElement>("BombSynthState");
    auto s = params_.copyState();
    if (auto px = s.createXml()) root->addChildElement(px.release());
    root->addChildElement(sequencer_.createXml());

    // Persist user wavetable file paths
    auto* wtElem = new juce::XmlElement("UserWavetables");
    for (const auto& path : userWavetablePaths_) {
        auto* fileElem = new juce::XmlElement("File");
        fileElem->setAttribute("path", path);
        wtElem->addChildElement(fileElem);
    }
    root->addChildElement(wtElem);

    copyXmlToBinary(*root, d);
}

void BombSynthAudioProcessor::setStateInformation(const void* d, int sz) {
    if (auto root = std::unique_ptr<juce::XmlElement>(getXmlFromBinary(d, sz))) {
        if (root->getTagName() == "BombSynthState") {
            if (auto* px = root->getFirstChildElement())
                params_.replaceState(juce::ValueTree::fromXml(*px));
            sequencer_.loadFromXml(root->getChildByName("Sequencer"));

            // Reload user wavetables
            if (auto* wtElem = root->getChildByName("UserWavetables")) {
                juce::AudioFormatManager mgr;
                mgr.registerBasicFormats();

                for (auto* fileElem : wtElem->getChildIterator()) {
                    juce::String path = fileElem->getStringAttribute("path");
                    juce::File   f(path);
                    if (!f.existsAsFile()) continue;

                    std::unique_ptr<juce::AudioFormatReader> reader(mgr.createReaderFor(f));
                    if (!reader) continue;

                    const int total = (int)reader->lengthInSamples;
                    if (total < 2) continue;

                    juce::AudioBuffer<float> wavBuf(1, total);
                    reader->read(&wavBuf, 0, total, 0, true, true);

                    WavetableBankLibrary::get().addUserBank(
                        f.getFileNameWithoutExtension(), 0xFFE040FB,
                        wavBuf.getReadPointer(0), total, path);

                    if (!userWavetablePaths_.contains(path))
                        userWavetablePaths_.add(path);
                }
            }
        } else {
            params_.replaceState(juce::ValueTree::fromXml(*root));
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BombSynthAudioProcessor(); }

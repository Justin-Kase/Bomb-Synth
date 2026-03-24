#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout BombSynthAudioProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master
    params.push_back(std::make_unique<juce::AudioParameterFloat>("master_gain",  "Master Gain",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.8f));

    // Amp envelope
    params.push_back(std::make_unique<juce::AudioParameterFloat>("amp_attack",   "Amp Attack",
        juce::NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 5.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("amp_decay",    "Amp Decay",
        juce::NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 100.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("amp_sustain",  "Amp Sustain",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("amp_release",  "Amp Release",
        juce::NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 300.f));

    // Filter
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter_cutoff", "Cutoff",
        juce::NormalisableRange<float>{20.f, 20000.f, 0.f, 0.25f}, 5000.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter_res",    "Resonance",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.f));

    // Filter envelope
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fenv_attack",  "Filter Attack",
        juce::NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 5.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fenv_decay",   "Filter Decay",
        juce::NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 200.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fenv_sustain", "Filter Sustain",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fenv_release", "Filter Release",
        juce::NormalisableRange<float>{0.1f, 10000.f, 0.f, 0.25f}, 500.f));

    // LFO 1
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo1_rate",  "LFO1 Rate",
        juce::NormalisableRange<float>{0.01f, 30.f, 0.f, 0.4f}, 1.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo1_depth", "LFO1 Depth",
        juce::NormalisableRange<float>{0.f, 1.f}, 0.f));

    return { params.begin(), params.end() };
}

BombSynthAudioProcessor::BombSynthAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{}

void BombSynthAudioProcessor::prepareToPlay(double sr, int blockSize) {
    engine_.prepare(sr, blockSize);
}

void BombSynthAudioProcessor::releaseResources() { engine_.reset(); }

#ifndef JucePlugin_PreferredChannelConfigurations
bool BombSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
#endif

void BombSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;

    // Pull parameters
    float masterGain  = *params_.getRawParameterValue("master_gain");
    float cutoff      = *params_.getRawParameterValue("filter_cutoff");
    float resonance   = *params_.getRawParameterValue("filter_res");

    ADSR::Params ampP;
    ampP.attackMs  = *params_.getRawParameterValue("amp_attack");
    ampP.decayMs   = *params_.getRawParameterValue("amp_decay");
    ampP.sustain   = *params_.getRawParameterValue("amp_sustain");
    ampP.releaseMs = *params_.getRawParameterValue("amp_release");

    ADSR::Params fenvP;
    fenvP.attackMs  = *params_.getRawParameterValue("fenv_attack");
    fenvP.decayMs   = *params_.getRawParameterValue("fenv_decay");
    fenvP.sustain   = *params_.getRawParameterValue("fenv_sustain");
    fenvP.releaseMs = *params_.getRawParameterValue("fenv_release");

    engine_.setMasterGain(masterGain);
    engine_.setCutoff(cutoff);
    engine_.setResonance(resonance);
    engine_.setAmpEnvParams(ampP);
    engine_.setFilterEnvParams(fenvP);

    engine_.processBlock(buffer, midi);
}

juce::AudioProcessorEditor* BombSynthAudioProcessor::createEditor() {
    return new BombSynthAudioProcessorEditor(*this);
}

void BombSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = params_.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void BombSynthAudioProcessor::setStateInformation(const void* data, int size) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml) params_.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new BombSynthAudioProcessor();
}

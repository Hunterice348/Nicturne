#include "PluginProcessor.h"
#include "PluginEditor.h"

HUNTERICE_SYNTHAudioProcessor::HUNTERICE_SYNTHAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

HUNTERICE_SYNTHAudioProcessor::~HUNTERICE_SYNTHAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout HUNTERICE_SYNTHAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Oscillator 1
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"osc1Wave", 1}, "Osc1 Wave", 0, 3, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc1Detune", 1}, "Osc1 Detune", juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));

    // Oscillator 2
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"osc2Wave", 1}, "Osc2 Wave", 0, 3, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc2Detune", 1}, "Osc2 Detune", juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 7.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc2Mix", 1}, "Osc2 Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    // ADSR
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"attack", 1}, "Attack", juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"decay", 1}, "Decay", juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"sustain", 1}, "Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"release", 1}, "Release", juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.3f));

    // Filter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"cutoff", 1}, "Cutoff", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 5000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"resonance", 1}, "Resonance", juce::NormalisableRange<float>(0.5f, 10.0f, 0.1f), 1.0f));

    // Master
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"masterGain", 1}, "Master", juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 0.8f));

    return { params.begin(), params.end() };
}

void HUNTERICE_SYNTHAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.prepare(sampleRate);
}

void HUNTERICE_SYNTHAudioProcessor::releaseResources() {}

void HUNTERICE_SYNTHAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Update parameters
    int osc1Wave = (int)apvts.getRawParameterValue("osc1Wave")->load();
    float osc1Detune = apvts.getRawParameterValue("osc1Detune")->load();
    int osc2Wave = (int)apvts.getRawParameterValue("osc2Wave")->load();
    float osc2Detune = apvts.getRawParameterValue("osc2Detune")->load();
    float osc2Mix = apvts.getRawParameterValue("osc2Mix")->load();
    float attack = apvts.getRawParameterValue("attack")->load();
    float decay = apvts.getRawParameterValue("decay")->load();
    float sustain = apvts.getRawParameterValue("sustain")->load();
    float release = apvts.getRawParameterValue("release")->load();
    float cutoff = apvts.getRawParameterValue("cutoff")->load();
    float reso = apvts.getRawParameterValue("resonance")->load();
    float masterGain = apvts.getRawParameterValue("masterGain")->load();

    synth.setOsc1(osc1Wave, osc1Detune);
    synth.setOsc2(osc2Wave, osc2Detune, osc2Mix);
    synth.setADSR(attack, decay, sustain, release);
    synth.setFilter(cutoff, reso);

    // Process MIDI
    for (const auto metadata : midi) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            synth.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        } else if (msg.isNoteOff()) {
            synth.noteOff(msg.getNoteNumber());
        }
    }

    // Render audio
    float* channelDataL = buffer.getWritePointer(0);
    float* channelDataR = buffer.getWritePointer(1);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float s = synth.render() * masterGain;
        channelDataL[sample] = s;
        channelDataR[sample] = s;
    }
}

juce::AudioProcessorEditor* HUNTERICE_SYNTHAudioProcessor::createEditor()
{
    return new HUNTERICE_SYNTHAudioProcessorEditor(*this);
}

void HUNTERICE_SYNTHAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HUNTERICE_SYNTHAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HUNTERICE_SYNTHAudioProcessor();
}

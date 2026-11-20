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
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto fparam = [&](const juce::String& id, const juce::String& name, float mn, float mx, float def, float skew=1.0f) {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>(mn, mx, 0.001f, skew), def));
    };
    auto iparam = [&](const juce::String& id, const juce::String& name, int mn, int mx, int def) {
        p.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{id, 1}, name, mn, mx, def));
    };

    iparam("osc1Wave", "Osc1 Wave", 0, 5, 1);
    fparam("osc1Detune", "Osc1 Detune", -100.0f, 100.0f, 0.0f);
    iparam("osc2Wave", "Osc2 Wave", 0, 5, 2);
    fparam("osc2Detune", "Osc2 Detune", -100.0f, 100.0f, 7.0f);
    fparam("osc2Mix", "Osc2 Mix", 0.0f, 1.0f, 0.5f);
    fparam("subMix", "Sub Mix", 0.0f, 1.0f, 0.0f);
    fparam("noiseMix", "Noise Mix", 0.0f, 1.0f, 0.0f);

    fparam("ampAttack", "Amp Attack", 0.001f, 5.0f, 0.01f, 0.3f);
    fparam("ampDecay", "Amp Decay", 0.001f, 5.0f, 0.2f, 0.3f);
    fparam("ampSustain", "Amp Sustain", 0.0f, 1.0f, 0.7f);
    fparam("ampRelease", "Amp Release", 0.001f, 10.0f, 0.3f, 0.3f);

    fparam("filtAttack", "Filt Attack", 0.001f, 5.0f, 0.01f, 0.3f);
    fparam("filtDecay", "Filt Decay", 0.001f, 5.0f, 0.2f, 0.3f);
    fparam("filtSustain", "Filt Sustain", 0.0f, 1.0f, 0.7f);
    fparam("filtRelease", "Filt Release", 0.001f, 10.0f, 0.3f, 0.3f);

    fparam("cutoff", "Cutoff", 20.0f, 20000.0f, 5000.0f, 0.3f);
    fparam("resonance", "Resonance", 0.5f, 10.0f, 1.0f);
    fparam("filterEnvAmt", "Filter Env", 0.0f, 1.0f, 0.0f);
    iparam("filterMode", "Filter Mode", 0, 3, 0);

    iparam("lfo1Shape", "LFO1 Shape", 0, 5, 0);
    iparam("lfo2Shape", "LFO2 Shape", 0, 5, 0);
    fparam("lfo1Rate", "LFO1 Rate", 0.01f, 30.0f, 1.0f, 0.3f);
    fparam("lfo2Rate", "LFO2 Rate", 0.01f, 30.0f, 1.0f, 0.3f);
    fparam("lfo1Depth", "LFO1 Depth", 0.0f, 1.0f, 0.0f);
    fparam("lfo2Depth", "LFO2 Depth", 0.0f, 1.0f, 0.0f);

    fparam("pitchMod", "Pitch Mod", 0.0f, 1.0f, 0.0f);
    fparam("filterMod", "Filter Mod", 0.0f, 1.0f, 0.0f);

    iparam("unison", "Unison", 1, 8, 1);
    fparam("unisonDetune", "Unison Detune", 0.0f, 50.0f, 10.0f);
    fparam("portamento", "Portamento", 0.0f, 1.0f, 0.0f, 0.3f);

    iparam("distMode", "Dist Mode", 0, 9, 1);
    fparam("distDrive", "Dist Drive", 0.1f, 20.0f, 1.0f, 0.3f);
    fparam("distMix", "Dist Mix", 0.0f, 1.0f, 0.0f);
    fparam("delayTime", "Delay Time", 0.0f, 2.0f, 0.3f, 0.3f);
    fparam("delayFb", "Delay FB", 0.0f, 0.95f, 0.4f);
    fparam("delayMix", "Delay Mix", 0.0f, 1.0f, 0.0f);
    fparam("reverbRoom", "Reverb Room", 0.0f, 1.0f, 0.5f);
    fparam("reverbDamp", "Reverb Damp", 0.0f, 1.0f, 0.5f);
    fparam("reverbMix", "Reverb Mix", 0.0f, 1.0f, 0.0f);
    fparam("chorusRate", "Chorus Rate", 0.01f, 10.0f, 1.0f, 0.3f);
    fparam("chorusDepth", "Chorus Depth", 0.0f, 1.0f, 0.3f);
    fparam("chorusMix", "Chorus Mix", 0.0f, 1.0f, 0.0f);

    fparam("masterGain", "Master", 0.0f, 2.0f, 0.8f);
    return { p.begin(), p.end() };
}

// ИСПРАВЛЕНО: добавлен [[maybe_unused]] для samplesPerBlock
void HUNTERICE_SYNTHAudioProcessor::prepareToPlay(double sampleRate, [[maybe_unused]] int samplesPerBlock)
{
    synth.prepare(sampleRate);
}

void HUNTERICE_SYNTHAudioProcessor::releaseResources() { synth.reset(); }

void HUNTERICE_SYNTHAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    auto get = [&](const juce::String& id) { return apvts.getRawParameterValue(id)->load(); };
    auto getI = [&](const juce::String& id) { return (int)get(id); };

    synth.applyToAll([&](hunterice::Voice& v) {
        v.setOsc1(getI("osc1Wave"), get("osc1Detune"), 0.5f);
        v.setOsc2(getI("osc2Wave"), get("osc2Detune"), 0.5f, get("osc2Mix"));
        v.setSub(get("subMix"), 1);
        v.setNoise(get("noiseMix"));
        v.setAmpADSR(get("ampAttack"), get("ampDecay"), get("ampSustain"), get("ampRelease"));
        v.setFilterADSR(get("filtAttack"), get("filtDecay"), get("filtSustain"), get("filtRelease"));
        v.setFilter(get("cutoff"), get("resonance"), getI("filterMode"), get("filterEnvAmt"));
        v.setLFO1(getI("lfo1Shape"), get("lfo1Rate"), get("lfo1Depth"));
        v.setLFO2(getI("lfo2Shape"), get("lfo2Rate"), get("lfo2Depth"));
        v.setPitchMod(get("pitchMod"));
        v.setFilterMod(get("filterMod"));
        v.setUnison(getI("unison"), get("unisonDetune"));
        v.setPortamento(get("portamento"));
    });

    for (const auto metadata : midi) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn()) synth.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff()) synth.noteOff(msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) synth.allNotesOff();
    }

    synth.setDistortion(getI("distMode"), get("distDrive"), get("distMix"));
    synth.setDelay(get("delayTime"), get("delayFb"), get("delayMix"));
    synth.setReverb(get("reverbRoom"), get("reverbDamp"), get("reverbMix"));
    synth.setChorus(get("chorusRate"), get("chorusDepth"), get("chorusMix"));
    synth.setMaster(get("masterGain"));

    float* L = buffer.getWritePointer(0);
    float* R = buffer.getWritePointer(1);
    synth.process(L, R, buffer.getNumSamples());
}

juce::AudioProcessorEditor* HUNTERICE_SYNTHAudioProcessor::createEditor()
{
    return new HUNTERICE_SYNTHAudioProcessorEditor(*this);
}

void HUNTERICE_SYNTHAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    xml->setAttribute("preset", presetManager.getCurrentPreset());
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

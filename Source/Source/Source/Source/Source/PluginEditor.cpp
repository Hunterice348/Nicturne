#include "PluginProcessor.h"
#include "PluginEditor.h"

HUNTERICE_SYNTHAudioProcessorEditor::HUNTERICE_SYNTHAudioProcessorEditor(HUNTERICE_SYNTHAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&lookAndFeel);
    setResizable(true, true);
    setResizeLimits(800, 500, 1400, 900);
    setSize(900, 600);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name,
                              const juce::String& paramID, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
        l.setText(name, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, paramID, s);
        addAndMakeVisible(s);
        addAndMakeVisible(l);
    };

    // Oscillators
    juce::StringArray waves = {"Sine", "Saw", "Square", "Triangle"};
    for (int i = 0; i < 4; ++i) {
        osc1WaveBox.addItem(waves[i], i + 1);
        osc2WaveBox.addItem(waves[i], i + 1);
    }
    osc1WaveBox.setSelectedId(2);
    osc2WaveBox.setSelectedId(3);
    osc1WaveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "osc1Wave", osc1WaveBox);
    osc2WaveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "osc2Wave", osc2WaveBox);
    addAndMakeVisible(osc1WaveBox);
    addAndMakeVisible(osc2WaveBox);

    setupSlider(osc1DetuneSlider, osc1Label, "OSC1", "osc1Detune", osc1DetuneAtt);
    setupSlider(osc2DetuneSlider, osc2Label, "OSC2 DET", "osc2Detune", osc2DetuneAtt);
    setupSlider(osc2MixSlider, osc2Label, "OSC2 MIX", "osc2Mix", osc2MixAtt);

    // Envelope
    setupSlider(attackSlider, envLabel, "ATTACK", "attack", attackAtt);
    setupSlider(decaySlider, envLabel, "DECAY", "decay", decayAtt);
    setupSlider(sustainSlider, envLabel, "SUSTAIN", "sustain", sustainAtt);
    setupSlider(releaseSlider, envLabel, "RELEASE", "release", releaseAtt);

    // Filter
    setupSlider(cutoffSlider, filterLabel, "CUTOFF", "cutoff", cutoffAtt);
    setupSlider(resSlider, filterLabel, "RESO", "resonance", resAtt);

    // Master
    setupSlider(masterSlider, masterLabel, "MASTER", "masterGain", masterAtt);
}

HUNTERICE_SYNTHAudioProcessorEditor::~HUNTERICE_SYNTHAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void HUNTERICE_SYNTHAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0a0c));
    g.setColour(juce::Colour(0xffcc0000));
    g.setFont(32.0f);
    g.drawText("HUNTERICE SYNTH", 20, 15, getWidth() - 40, 40, juce::Justification::centred);
}

void HUNTERICE_SYNTHAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(60);

    auto oscArea = area.removeFromTop(150);
    osc1WaveBox.setBounds(oscArea.removeFromLeft(120).reduced(10));
    osc2WaveBox.setBounds(oscArea.removeFromLeft(120).reduced(10));

    auto knobRow = area.removeFromTop(120);
    auto kw = knobRow.getWidth() / 8;
    osc1DetuneSlider.setBounds(knobRow.removeFromLeft(kw).reduced(5));
    osc2DetuneSlider.setBounds(knobRow.removeFromLeft(kw).reduced(5));
    osc2MixSlider.setBounds(knobRow.removeFromLeft(kw).reduced(5));
    attackSlider.setBounds(knobRow.removeFromLeft(kw).reduced(5));
    decaySlider.setBounds(knobRow.removeFromLeft(kw).reduced(5));
    sustainSlider.setBounds(knobRow.removeFromLeft(kw).reduced(5));
    releaseSlider.setBounds(knobRow.removeFromLeft(kw).reduced(5));
    cutoffSlider.setBounds(knobRow.removeFromLeft(kw).reduced(5));

    auto knobRow2 = area.removeFromTop(120);
    resSlider.setBounds(knobRow2.removeFromLeft(kw).reduced(5));
    masterSlider.setBounds(knobRow2.removeFromLeft(kw).reduced(5));
}

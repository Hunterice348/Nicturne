#include "PluginProcessor.h"
#include "PluginEditor.h"

HUNTERICE_SYNTHAudioProcessorEditor::HUNTERICE_SYNTHAudioProcessorEditor(HUNTERICE_SYNTHAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&lookAndFeel);
    setResizable(true, true);
    setResizeLimits(1200, 800, 1920, 1080);
    setSize(1400, 850);

    // ИСПРАВЛЕНИЕ: используем audioProcessor.apvts вместо p.apvts
    auto makeSlider = [this](juce::Slider& s, const juce::String& paramID,
                             std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, paramID, s);
        addAndMakeVisible(s);
    };

    // Oscillators
    juce::StringArray waves = {"Sine", "Triangle", "Saw", "Square", "Pulse", "Noise"};
    for (int i = 0; i < 6; ++i) { osc1WaveBox.addItem(waves[i], i + 1); osc2WaveBox.addItem(waves[i], i + 1); }
    osc1WaveBox.setSelectedId(2); osc2WaveBox.setSelectedId(3);
    osc1WaveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "osc1Wave", osc1WaveBox);
    osc2WaveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "osc2Wave", osc2WaveBox);
    addAndMakeVisible(osc1WaveBox); addAndMakeVisible(osc2WaveBox);

    makeSlider(osc1Detune, "osc1Detune", osc1DetuneAtt);
    makeSlider(osc2Detune, "osc2Detune", osc2DetuneAtt);
    makeSlider(osc2Mix, "osc2Mix", osc2MixAtt);
    makeSlider(subMix, "subMix", subMixAtt);
    makeSlider(noiseMix, "noiseMix", noiseMixAtt);

    // Amp Envelope
    makeSlider(ampA, "ampAttack", ampAAtt); makeSlider(ampD, "ampDecay", ampDAtt);
    makeSlider(ampS, "ampSustain", ampSAtt); makeSlider(ampR, "ampRelease", ampRAtt);

    // Filter Envelope
    makeSlider(filtA, "filtAttack", filtAAtt); makeSlider(filtD, "filtDecay", filtDAtt);
    makeSlider(filtS, "filtSustain", filtSAtt); makeSlider(filtR, "filtRelease", filtRAtt);

    // Filter
    makeSlider(cutoff, "cutoff", cutoffAtt); makeSlider(reso, "resonance", resoAtt);
    makeSlider(filtEnv, "filterEnvAmt", filtEnvAtt);
    juce::StringArray fmodes = {"LP", "HP", "BP", "Notch"};
    for (int i = 0; i < 4; ++i) filterModeBox.addItem(fmodes[i], i + 1);
    filterModeBox.setSelectedId(1);
    filterModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "filterMode", filterModeBox);
    addAndMakeVisible(filterModeBox);

    // LFO
    juce::StringArray lfoShapes = {"Sine", "Triangle", "Saw", "Square", "Random", "S&H"};
    for (int i = 0; i < 6; ++i) { lfo1ShapeBox.addItem(lfoShapes[i], i + 1); lfo2ShapeBox.addItem(lfoShapes[i], i + 1); }
    lfo1ShapeBox.setSelectedId(1); lfo2ShapeBox.setSelectedId(1);
    lfo1ShapeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "lfo1Shape", lfo1ShapeBox);
    lfo2ShapeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "lfo2Shape", lfo2ShapeBox);
    addAndMakeVisible(lfo1ShapeBox); addAndMakeVisible(lfo2ShapeBox);
    makeSlider(lfo1Rate, "lfo1Rate", lfo1RateAtt); makeSlider(lfo2Rate, "lfo2Rate", lfo2RateAtt);
    makeSlider(lfo1Depth, "lfo1Depth", lfo1DepthAtt); makeSlider(lfo2Depth, "lfo2Depth", lfo2DepthAtt);

    // Mod
    makeSlider(pitchMod, "pitchMod", pitchModAtt); makeSlider(filterMod, "filterMod", filterModAtt);
    makeSlider(unison, "unison", unisonAtt); makeSlider(unisonDetune, "unisonDetune", unisonDetuneAtt);
    makeSlider(porta, "portamento", portaAtt);

    // FX
    juce::StringArray distModes = {"Hard","Soft","Tube","Diode","Fuzz","Fold","Wave","Asym","Digital","Industrial"};
    for (int i = 0; i < 10; ++i) distModeBox.addItem(distModes[i], i + 1);
    distModeBox.setSelectedId(2);
    distModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "distMode", distModeBox);
    addAndMakeVisible(distModeBox);
    makeSlider(distDrive, "distDrive", distDriveAtt); makeSlider(distMix, "distMix", distMixAtt);
    makeSlider(delayTime, "delayTime", delayTimeAtt); makeSlider(delayFb, "delayFb", delayFbAtt);
    makeSlider(delayMix, "delayMix", delayMixAtt);
    makeSlider(reverbRoom, "reverbRoom", reverbRoomAtt); makeSlider(reverbDamp, "reverbDamp", reverbDampAtt);
    makeSlider(reverbMix, "reverbMix", reverbMixAtt);
    makeSlider(chorusRate, "chorusRate", chorusRateAtt); makeSlider(chorusDepth, "chorusDepth", chorusDepthAtt);
    makeSlider(chorusMix, "chorusMix", chorusMixAtt);

    // Master
    makeSlider(master, "masterGain", masterAtt);

    // Preset
    for (int i = 0; i < (int)audioProcessor.presetManager.getPresets().size(); ++i) {
        presetBox.addItem(audioProcessor.presetManager.getPresets()[i].name, i + 1);
    }
    presetBox.onChange = [this]() {
        int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx < (int)audioProcessor.presetManager.getPresets().size()) {
            audioProcessor.presetManager.applyPreset(audioProcessor.presetManager.getPresets()[idx], audioProcessor.apvts);
        }
    };
    addAndMakeVisible(presetBox);

    startTimer(100);
}

HUNTERICE_SYNTHAudioProcessorEditor::~HUNTERICE_SYNTHAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void HUNTERICE_SYNTHAudioProcessorEditor::timerCallback() { repaint(); }

void HUNTERICE_SYNTHAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ColourGradient bg(juce::Colour(0xff15151a), 0, 0, juce::Colour(0xff050507), 0, (float)getHeight(), false);
    g.setGradientFill(bg);
    g.fillAll();

    g.setColour(juce::Colour(0xffcc0000));
    g.setFont(juce::Font(36.0f, juce::Font::bold));
    g.drawText("HUNTERICE", 20, 10, 300, 40, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff666666));
    g.setFont(14.0f);
    g.drawText("PROFESSIONAL SYNTHESIZER", 20, 45, 300, 20, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xffcc0000));
    g.drawLine(20.0f, 70.0f, (float)getWidth() - 20.0f, 70.0f, 1.5f);
}

void HUNTERICE_SYNTHAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(15);
    area.removeFromTop(80);

    auto presetBar = area.removeFromTop(35);
    presetBox.setBounds(presetBar.removeFromLeft(400).reduced(2));
    area.removeFromTop(10);

    auto col1 = area.removeFromLeft(area.getWidth() / 3).reduced(5);
    auto col2 = area.removeFromLeft(area.getWidth() / 2).reduced(5);
    auto col3 = area.reduced(5);

    auto oscArea = col1.removeFromTop(col1.getHeight() / 2).reduced(5);
    auto envArea = col1.reduced(5);

    auto oscRow1 = oscArea.removeFromTop(30);
    osc1WaveBox.setBounds(oscRow1.removeFromLeft(120).reduced(2));
    osc2WaveBox.setBounds(oscRow1.removeFromLeft(120).reduced(2));

    auto knobRow = oscArea.removeFromTop(110);
    auto kw = knobRow.getWidth() / 5;
    osc1Detune.setBounds(knobRow.removeFromLeft(kw).reduced(3));
    osc2Detune.setBounds(knobRow.removeFromLeft(kw).reduced(3));
    osc2Mix.setBounds(knobRow.removeFromLeft(kw).reduced(3));
    subMix.setBounds(knobRow.removeFromLeft(kw).reduced(3));
    noiseMix.setBounds(knobRow.removeFromLeft(kw).reduced(3));

    auto envRow1 = envArea.removeFromTop(110);
    auto ew = envRow1.getWidth() / 4;
    ampA.setBounds(envRow1.removeFromLeft(ew).reduced(3));
    ampD.setBounds(envRow1.removeFromLeft(ew).reduced(3));
    ampS.setBounds(envRow1.removeFromLeft(ew).reduced(3));
    ampR.setBounds(envRow1.removeFromLeft(ew).reduced(3));
    
    auto envRow2 = envArea.removeFromTop(110);
    filtA.setBounds(envRow2.removeFromLeft(ew).reduced(3));
    filtD.setBounds(envRow2.removeFromLeft(ew).reduced(3));
    filtS.setBounds(envRow2.removeFromLeft(ew).reduced(3));
    filtR.setBounds(envRow2.removeFromLeft(ew).reduced(3));

    auto filtArea = col2.removeFromTop(col2.getHeight() / 3).reduced(5);
    auto lfoArea = col2.removeFromTop(col2.getHeight() / 2).reduced(5);
    auto modArea = col2.reduced(5);

    auto filtRow1 = filtArea.removeFromTop(30);
    filterModeBox.setBounds(filtRow1.removeFromLeft(120).reduced(2));
    auto filtRow2 = filtArea.removeFromTop(110);
    auto fw = filtRow2.getWidth() / 3;
    cutoff.setBounds(filtRow2.removeFromLeft(fw).reduced(3));
    reso.setBounds(filtRow2.removeFromLeft(fw).reduced(3));
    filtEnv.setBounds(filtRow2.removeFromLeft(fw).reduced(3));

    auto lfoRow1 = lfoArea.removeFromTop(30);
    lfo1ShapeBox.setBounds(lfoRow1.removeFromLeft(100).reduced(2));
    lfo2ShapeBox.setBounds(lfoRow1.removeFromLeft(100).reduced(2));
    auto lfoRow2 = lfoArea.removeFromTop(110);
    auto lw = lfoRow2.getWidth() / 4;
    lfo1Rate.setBounds(lfoRow2.removeFromLeft(lw).reduced(3));
    lfo2Rate.setBounds(lfoRow2.removeFromLeft(lw).reduced(3));
    lfo1Depth.setBounds(lfoRow2.removeFromLeft(lw).reduced(3));
    lfo2Depth.setBounds(lfoRow2.removeFromLeft(lw).reduced(3));

    auto modRow = modArea.removeFromTop(110);
    auto mw = modRow.getWidth() / 5;
    pitchMod.setBounds(modRow.removeFromLeft(mw).reduced(3));
    filterMod.setBounds(modRow.removeFromLeft(mw).reduced(3));
    unison.setBounds(modRow.removeFromLeft(mw).reduced(3));
    unisonDetune.setBounds(modRow.removeFromLeft(mw).reduced(3));
    porta.setBounds(modRow.removeFromLeft(mw).reduced(3));

    auto fxArea = col3.removeFromTop(col3.getHeight() * 2 / 3).reduced(5);
    auto masterArea = col3.reduced(5);

    auto distRow = fxArea.removeFromTop(30);
    distModeBox.setBounds(distRow.removeFromLeft(150).reduced(2));
    auto distKnobs = fxArea.removeFromTop(110);
    auto dw = distKnobs.getWidth() / 3; // Исправлено: было 2, стало 3 для Drive, Mix и пустого места или можно добавить Output
    distDrive.setBounds(distKnobs.removeFromLeft(dw).reduced(3));
    distMix.setBounds(distKnobs.removeFromLeft(dw).reduced(3));

    auto delayKnobs = fxArea.removeFromTop(110);
    auto dyw = delayKnobs.getWidth() / 3;
    delayTime.setBounds(delayKnobs.removeFromLeft(dyw).reduced(3));
    delayFb.setBounds(delayKnobs.removeFromLeft(dyw).reduced(3));
    delayMix.setBounds(delayKnobs.removeFromLeft(dyw).reduced(3));

    auto revKnobs = fxArea.removeFromTop(110);
    auto rvw = revKnobs.getWidth() / 3;
    reverbRoom.setBounds(revKnobs.removeFromLeft(rvw).reduced(3));
    reverbDamp.setBounds(revKnobs.removeFromLeft(rvw).reduced(3));
    reverbMix.setBounds(revKnobs.removeFromLeft(rvw).reduced(3));

    auto chorusKnobs = fxArea.removeFromTop(110);
    auto chw = chorusKnobs.getWidth() / 3;
    chorusRate.setBounds(chorusKnobs.removeFromLeft(chw).reduced(3));
    chorusDepth.setBounds(chorusKnobs.removeFromLeft(chw).reduced(3));
    chorusMix.setBounds(chorusKnobs.removeFromLeft(chw).reduced(3));

    auto masterKnob = masterArea.removeFromTop(150);
    master.setBounds(masterKnob.reduced(30));
}

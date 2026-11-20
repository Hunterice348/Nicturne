#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class Hunterice3DLookAndFeel : public juce::LookAndFeel_V4 {
public:
    Hunterice3DLookAndFeel() {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff0a0a0c));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a1a));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xffcc0000));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff333333));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto cx = bounds.getCentreX();
        auto cy = bounds.getCentreY();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Shadow
        juce::DropShadow shadow(juce::Colours::black.withAlpha(0.7f), 8, {0, 4});
        shadow.drawForRectangle(g, juce::Rectangle<int>((int)(cx - radius), (int)(cy - radius),
                                                         (int)(radius * 2), (int)(radius * 2)));

        // Outer metallic ring
        juce::ColourGradient ringGrad(juce::Colour(0xff555555), cx, cy - radius,
                                       juce::Colour(0xff111111), cx, cy + radius, false);
        g.setGradientFill(ringGrad);
        g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);

        // Inner knob
        juce::ColourGradient knobGrad(juce::Colour(0xff3a3a3a), cx, cy - radius * 0.85f,
                                       juce::Colour(0xff0a0a0a), cx, cy + radius * 0.85f, false);
        g.setGradientFill(knobGrad);
        g.fillEllipse(cx - radius * 0.85f, cy - radius * 0.85f, radius * 1.7f, radius * 1.7f);

        // Highlight
        g.setColour(juce::Colour(0x30ffffff));
        g.fillEllipse(cx - radius * 0.5f, cy - radius * 0.75f, radius * 0.8f, radius * 0.3f);

        // Pointer
        juce::Path p;
        p.addRectangle(cx - 2.0f, cy - radius * 0.75f, 4.0f, radius * 0.5f);
        g.setColour(juce::Colour(0xffcc0000));
        g.fillPath(p, juce::AffineTransform::rotation(angle, cx, cy));

        // Arc
        juce::Path arc;
        arc.addCentredArc(cx, cy, radius * 1.05f, radius * 1.05f, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(juce::Colour(0xffcc0000));
        g.strokePath(arc, juce::PathStrokeType(2.5f));
    }
};

class HUNTERICE_SYNTHAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
    HUNTERICE_SYNTHAudioProcessorEditor(HUNTERICE_SYNTHAudioProcessor&);
    ~HUNTERICE_SYNTHAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    HUNTERICE_SYNTHAudioProcessor& audioProcessor;
    Hunterice3DLookAndFeel lookAndFeel;

    // Sections
    juce::Component oscSection, envSection, filtSection, lfoSection, modSection, fxSection, masterSection;

    // Osc
    juce::ComboBox osc1WaveBox, osc2WaveBox;
    juce::Slider osc1Detune, osc2Detune, osc2Mix, subMix, noiseMix;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc1WaveAtt, osc2WaveAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc1DetuneAtt, osc2DetuneAtt, osc2MixAtt, subMixAtt, noiseMixAtt;

    // Env
    juce::Slider ampA, ampD, ampS, ampR, filtA, filtD, filtS, filtR;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampAAtt, ampDAtt, ampSAtt, ampRAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filtAAtt, filtDAtt, filtSAtt, filtRAtt;

    // Filter
    juce::Slider cutoff, reso, filtEnv;
    juce::ComboBox filterModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAtt, resoAtt, filtEnvAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterModeAtt;

    // LFO
    juce::ComboBox lfo1ShapeBox, lfo2ShapeBox;
    juce::Slider lfo1Rate, lfo2Rate, lfo1Depth, lfo2Depth;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo1ShapeAtt, lfo2ShapeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo1RateAtt, lfo2RateAtt, lfo1DepthAtt, lfo2DepthAtt;

    // Mod
    juce::Slider pitchMod, filterMod, unison, unisonDetune, porta;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchModAtt, filterModAtt, unisonAtt, unisonDetuneAtt, portaAtt;

    // FX
    juce::ComboBox distModeBox;
    juce::Slider distDrive, distMix, delayTime, delayFb, delayMix, reverbRoom, reverbDamp, reverbMix, chorusRate, chorusDepth, chorusMix;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> distModeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> distDriveAtt, distMixAtt, delayTimeAtt, delayFbAtt, delayMixAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbRoomAtt, reverbDampAtt, reverbMixAtt, chorusRateAtt, chorusDepthAtt, chorusMixAtt;

    // Master
    juce::Slider master;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAtt;

    // Preset
    juce::ComboBox presetBox;
    juce::Label presetLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HUNTERICE_SYNTHAudioProcessorEditor)
};

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class HuntericeLookAndFeel : public juce::LookAndFeel_V4 {
public:
    HuntericeLookAndFeel() {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff0a0a0c));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // 3D knob
        juce::ColourGradient grad(
            juce::Colour(0xff3a3a3a), centreX, centreY - radius * 0.85f,
            juce::Colour(0xff0a0a0a), centreX, centreY + radius * 0.85f, false);
        g.setGradientFill(grad);
        g.fillEllipse(centreX - radius * 0.85f, centreY - radius * 0.85f, radius * 1.7f, radius * 1.7f);

        // Pointer
        juce::Path p;
        p.addRectangle(centreX - 2.0f, centreY - radius * 0.75f, 4.0f, radius * 0.5f);
        g.setColour(juce::Colour(0xffcc0000));
        g.fillPath(p, juce::AffineTransform::rotation(angle, centreX, centreY));
    }
};

class HUNTERICE_SYNTHAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    HUNTERICE_SYNTHAudioProcessorEditor(HUNTERICE_SYNTHAudioProcessor&);
    ~HUNTERICE_SYNTHAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    HUNTERICE_SYNTHAudioProcessor& audioProcessor;
    HuntericeLookAndFeel lookAndFeel;

    juce::ComboBox osc1WaveBox, osc2WaveBox;
    juce::Slider osc1DetuneSlider, osc2DetuneSlider, osc2MixSlider;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Slider cutoffSlider, resSlider, masterSlider;
    juce::Label osc1Label, osc2Label, envLabel, filterLabel, masterLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc1WaveAtt, osc2WaveAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc1DetuneAtt, osc2DetuneAtt, osc2MixAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAtt, decayAtt, sustainAtt, releaseAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAtt, resAtt, masterAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HUNTERICE_SYNTHAudioProcessorEditor)
};

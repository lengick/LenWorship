#pragma once

#include "NeonLookAndFeel.h"
#include "PluginProcessor.h"

class LenWorshipAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit LenWorshipAudioProcessorEditor (LenWorshipAudioProcessor&);
    ~LenWorshipAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct Panel
    {
        juce::String name;
        juce::Colour colour;
        int firstKnob;
        int numKnobs;
        juce::Rectangle<int> bounds;
    };

    LenWorshipAudioProcessor& owner;
    NeonLookAndFeel lnf;
    std::array<Knob, 8> knobs;
    std::array<Panel, 5> panels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LenWorshipAudioProcessorEditor)
};

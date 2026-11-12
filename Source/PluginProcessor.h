#pragma once

#include "Common.h"
#include "FxChain.h"
#include "Voices.h"

class LenWorshipAudioProcessor : public juce::AudioProcessor
{
public:
    LenWorshipAudioProcessor();
    ~LenWorshipAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Len-Worship"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::Synthesiser synth;
    juce::dsp::StateVariableTPTFilter<float> filter;
    lenfx::PingPongDelay delay;
    lenfx::BigReverb reverb;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> cutoffSmoothed;
    juce::SmoothedValue<float> volumeSmoothed;

    std::atomic<float>* pianoP = nullptr;
    std::atomic<float>* padP = nullptr;
    std::atomic<float>* cutoffP = nullptr;
    std::atomic<float>* delayP = nullptr;
    std::atomic<float>* delayTimeP = nullptr;
    std::atomic<float>* reverbP = nullptr;
    std::atomic<float>* sizeP = nullptr;
    std::atomic<float>* volumeP = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LenWorshipAudioProcessor)
};

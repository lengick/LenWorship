#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr int kNumVoices = 24;

juce::String percentText (float v, int) { return juce::String ((int) std::round (v * 100.0f)) + " %"; }

juce::String hzText (float v, int)
{
    return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " kHz"
                        : juce::String ((int) std::round (v)) + " Hz";
}

juce::String msText (float v, int) { return juce::String ((int) std::round (v)) + " ms"; }
} // namespace

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout LenWorshipAudioProcessor::createLayout()
{
    using juce::AudioParameterFloat;
    using juce::AudioParameterFloatAttributes;
    using juce::NormalisableRange;
    using juce::ParameterID;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto unit = NormalisableRange<float> (0.0f, 1.0f, 0.001f);

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "piano", 1 }, "Piano", unit, 0.80f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "pad", 1 }, "Pad", unit, 0.50f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "cutoff", 1 }, "Cutoff",
                                                       NormalisableRange<float> (40.0f, 20000.0f, 0.0f, 0.3f), 4500.0f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (hzText)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "delay", 1 }, "Delay", unit, 0.25f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "delayTime", 1 }, "Delay Time",
                                                       NormalisableRange<float> (50.0f, 1200.0f, 1.0f, 0.6f), 450.0f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (msText)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "reverb", 1 }, "Reverb", unit, 0.45f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "reverbSize", 1 }, "Reverb Size", unit, 0.75f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume", unit, 0.70f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));

    return layout;
}

//==============================================================================
LenWorshipAudioProcessor::LenWorshipAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pianoP = apvts.getRawParameterValue ("piano");
    padP = apvts.getRawParameterValue ("pad");
    cutoffP = apvts.getRawParameterValue ("cutoff");
    delayP = apvts.getRawParameterValue ("delay");
    delayTimeP = apvts.getRawParameterValue ("delayTime");
    reverbP = apvts.getRawParameterValue ("reverb");
    sizeP = apvts.getRawParameterValue ("reverbSize");
    volumeP = apvts.getRawParameterValue ("volume");

    synth.addSound (new WorshipSound());
    for (int i = 0; i < kNumVoices; ++i)
        synth.addVoice (new WorshipVoice());
    synth.setNoteStealingEnabled (true);
}

void LenWorshipAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 };
    filter.prepare (spec);
    filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.setResonance (0.85f);
    filter.reset();

    delay.prepare (sampleRate);
    reverb.prepare (sampleRate);

    cutoffSmoothed.reset (sampleRate, 0.03);
    cutoffSmoothed.setCurrentAndTargetValue (cutoffP->load());
    volumeSmoothed.reset (sampleRate, 0.03);
    volumeSmoothed.setCurrentAndTargetValue (volumeP->load());
}

bool LenWorshipAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void LenWorshipAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    if (buffer.getNumChannels() < 2)
        return;

    // --- niveles de las dos capas
    const float pianoLevel = pianoP->load();
    const float padLevel = padP->load();
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<WorshipVoice*> (synth.getVoice (i)))
            v->setLevels (pianoLevel, padLevel);

    // --- sintesis
    synth.renderNextBlock (buffer, midi, 0, numSamples);

    // --- parametros de efectos
    cutoffSmoothed.setTargetValue (juce::jmax (20.0f, cutoffP->load()));
    const float vol = volumeP->load();
    volumeSmoothed.setTargetValue (vol * vol * 2.0f);

    delay.setMix (delayP->load());
    delay.setTimeMs (delayTimeP->load());
    reverb.setMix (reverbP->load());
    reverb.setSize (sizeP->load());

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    const float maxCutoff = (float) (getSampleRate() * 0.45);

    for (int i = 0; i < numSamples; ++i)
    {
        const float cut = cutoffSmoothed.getNextValue();
        if ((i & 7) == 0)
            filter.setCutoffFrequency (juce::jmin (cut, maxCutoff));

        float l = filter.processSample (0, L[i]);
        float r = filter.processSample (1, R[i]);

        delay.process (l, r);
        reverb.process (l, r);

        const float g = volumeSmoothed.getNextValue();
        L[i] = std::tanh (l * g);
        R[i] = std::tanh (r * g);
    }
}

//==============================================================================
juce::AudioProcessorEditor* LenWorshipAudioProcessor::createEditor()
{
    return new LenWorshipAudioProcessorEditor (*this);
}

void LenWorshipAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void LenWorshipAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LenWorshipAudioProcessor();
}

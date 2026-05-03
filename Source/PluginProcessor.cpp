#include "PluginProcessor.h"
#include "PluginEditor.h"

TheMidsRoomAudioProcessor::TheMidsRoomAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    initializeSineTable();
}

TheMidsRoomAudioProcessor::~TheMidsRoomAudioProcessor() {}

void TheMidsRoomAudioProcessor::initializeSineTable()
{
    for (int i = 0; i < sineTableSize; ++i)
        sineTable[i] = std::sin(juce::MathConstants<float>::twoPi * (float)i / (float)sineTableSize);
}

inline float TheMidsRoomAudioProcessor::fastSine(float phase) const
{
    float p = phase * (float)sineTableSize / juce::MathConstants<float>::twoPi;
    int i = (int)p & (sineTableSize - 1);
    return sineTable[i];
}

juce::AudioProcessorValueTreeState::ParameterLayout TheMidsRoomAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("midLevel", 1), "Mid Level", 0.0f, 1.0f, 0.8f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sideLevel", 1), "Side Level", 0.0f, 1.0f, 0.7f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("extraWidth", 1), "Extra Width", 0.0f, 1.0f, 0.2f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("space", 1), "Space", 0.0f, 1.0f, 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("air", 1), "Air", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("move", 1), "Move", 0.0f, 1.0f, 0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("mix", 1), "Mix", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("doubleTrack", 1), "Double Track", false));
    return layout;
}

void TheMidsRoomAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    lastSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (uint32_t)samplesPerBlock;
    spec.numChannels = 1;

    sideHPF.prepare(spec);
    sideHPF.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 120.0f);
    sideReverb.prepare(spec);

    dtHighCut.prepare(spec);
    dtHighCut.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 6000.0f);

    const int delaySize = 8192;
    delayMask = delaySize - 1;
    sideDelayBuffer.setSize(1, delaySize);
    sideDelayBuffer.clear();
    dtDelayBuffer.setSize(1, delaySize);
    dtDelayBuffer.clear();

    midLevelSmoother.reset(sampleRate, 0.05);
    sideLevelSmoother.reset(sampleRate, 0.05);
    extraWidthSmoother.reset(sampleRate, 0.05);
    spaceSmoother.reset(sampleRate, 0.05);
    airSmoother.reset(sampleRate, 0.05);
    moveSmoother.reset(sampleRate, 0.05);
    mixSmoother.reset(sampleRate, 0.05);
    dtGainSmoother.reset(sampleRate, 0.05);
}

void TheMidsRoomAudioProcessor::releaseResources() {}

void TheMidsRoomAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    // 1. Update Targets
    midLevelSmoother.setTargetValue(apvts.getRawParameterValue("midLevel")->load());
    sideLevelSmoother.setTargetValue(apvts.getRawParameterValue("sideLevel")->load());
    extraWidthSmoother.setTargetValue(apvts.getRawParameterValue("extraWidth")->load());
    spaceSmoother.setTargetValue(apvts.getRawParameterValue("space")->load());
    airSmoother.setTargetValue(apvts.getRawParameterValue("air")->load());
    moveSmoother.setTargetValue(apvts.getRawParameterValue("move")->load());
    mixSmoother.setTargetValue(apvts.getRawParameterValue("mix")->load());
    dtGainSmoother.setTargetValue(apvts.getRawParameterValue("doubleTrack")->load() > 0.5f ? 1.0f : 0.0f);

    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.5f + (spaceSmoother.getTargetValue() * 0.3f);
    reverbParams.damping = 1.0f - airSmoother.getTargetValue();
    reverbParams.wetLevel = spaceSmoother.getTargetValue() * 0.5f;
    reverbParams.dryLevel = 1.0f;
    sideReverb.setParameters(reverbParams);

    // 2. M/S Extraction
    juce::AudioBuffer<float> midBuffer(1, numSamples);
    auto* midData = midBuffer.getWritePointer(0);
    auto* leftIn = buffer.getReadPointer(0);
    auto* rightIn = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : leftIn;
    for (int i = 0; i < numSamples; ++i) midData[i] = (leftIn[i] + rightIn[i]) * 0.5f;

    // 3. Side Engine
    juce::AudioBuffer<float> sideBuffer(1, numSamples);
    auto* sideData = sideBuffer.getWritePointer(0);
    auto* sideDelayData = sideDelayBuffer.getWritePointer(0);
    const float sr_ms = (float)lastSampleRate / 1000.0f;
    const float phaseInc = (juce::MathConstants<float>::twoPi * 0.15f) / (float)lastSampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        sideDelayData[sideWritePos] = midData[i];
        float move = moveSmoother.getNextValue() * 0.15f; 
        float mod = fastSine(sidePhase) * move;
        
        float rp1 = (float)sideWritePos - ((14.0f + mod) * sr_ms);
        float rp2 = (float)sideWritePos - ((19.0f - mod) * sr_ms);

        int i1 = (int)rp1; float f1 = rp1 - (float)i1;
        int i2 = (int)rp2; float f2 = rp2 - (float)i2;
        float v1 = sideDelayData[i1 & delayMask] * (1.0f - f1) + sideDelayData[(i1 + 1) & delayMask] * f1;
        float v2 = sideDelayData[i2 & delayMask] * (1.0f - f2) + sideDelayData[(i2 + 1) & delayMask] * f2;

        sideData[i] = (v1 + v2) * 0.5f * (1.0f + (extraWidthSmoother.getNextValue() * 0.5f));
        sideWritePos = (sideWritePos + 1) & delayMask;
        sidePhase += phaseInc;
    }

    juce::dsp::AudioBlock<float> sideBlock(sideBuffer);
    sideHPF.process(juce::dsp::ProcessContextReplacing<float>(sideBlock));
    sideReverb.process(juce::dsp::ProcessContextReplacing<float>(sideBlock));

    // 4. Double Track Path
    juce::AudioBuffer<float> dtBuffer(1, numSamples);
    auto* dtData = dtBuffer.getWritePointer(0);
    auto* dtDelayData = dtDelayBuffer.getWritePointer(0);
    const float dtPhaseInc = (juce::MathConstants<float>::twoPi * 0.25f) / (float)lastSampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        dtDelayData[dtWritePos] = midData[i];
        float mod = fastSine(dtPhase) * (moveSmoother.getCurrentValue() * 0.3f);
        float rp = (float)dtWritePos - ((25.0f + mod) * sr_ms);
        int i1 = (int)rp; float f1 = rp - (float)i1;
        dtData[i] = dtDelayData[i1 & delayMask] * (1.0f - f1) + dtDelayData[(i1 + 1) & delayMask] * f1;
        dtWritePos = (dtWritePos + 1) & delayMask;
        dtPhase += dtPhaseInc;
    }
    juce::dsp::AudioBlock<float> dtBlock(dtBuffer);
    dtHighCut.process(juce::dsp::ProcessContextReplacing<float>(dtBlock));

    // 5. Final Assembly
    auto* leftOut = buffer.getWritePointer(0);
    auto* rightOut = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : leftOut;
    const float midAtten = 1.0f - (spaceSmoother.getTargetValue() * 0.2f);
    auto* sideOut = sideBuffer.getReadPointer(0);
    auto* dtOut = dtBuffer.getReadPointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        float m = midData[i] * midLevelSmoother.getNextValue() * midAtten;
        float s = sideOut[i] * sideLevelSmoother.getNextValue();
        float d = dtOut[i] * dtGainSmoother.getNextValue() * 0.85f;

        float wetL = m + s + (d * 0.4f);
        float wetR = m - s + (d * 0.6f);

        float mix = mixSmoother.getNextValue();
        leftOut[i] = leftIn[i] * (1.0f - mix) + wetL * mix;
        if (buffer.getNumChannels() > 1)
            rightOut[i] = rightIn[i] * (1.0f - mix) + wetR * mix;
    }
}

juce::AudioProcessorEditor* TheMidsRoomAudioProcessor::createEditor() { return new TheMidsRoomAudioProcessorEditor (*this); }
void TheMidsRoomAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}
void TheMidsRoomAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TheMidsRoomAudioProcessor(); }

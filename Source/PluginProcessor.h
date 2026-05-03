#pragma once
#include <JuceHeader.h>

class TheMidsRoomAudioProcessor  : public juce::AudioProcessor
{
public:
    TheMidsRoomAudioProcessor();
    ~TheMidsRoomAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "THE MIDS ROOM"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double lastSampleRate = 44100.0;
    
    // Smoothers
    juce::LinearSmoothedValue<float> midLevelSmoother, sideLevelSmoother, extraWidthSmoother, spaceSmoother, airSmoother, moveSmoother, mixSmoother, dtGainSmoother;

    // Side Path
    juce::dsp::IIR::Filter<float> sideHPF;
    juce::dsp::Reverb sideReverb;
    juce::AudioBuffer<float> sideDelayBuffer;
    int sideWritePos = 0;
    float sidePhase = 0.0f;

    // Double Track Path
    juce::AudioBuffer<float> dtDelayBuffer;
    int dtWritePos = 0;
    float dtPhase = 0.0f;
    juce::dsp::IIR::Filter<float> dtHighCut;

    // Optimization: Wavetable LFO
    static constexpr int sineTableSize = 2048;
    float sineTable[sineTableSize];
    void initializeSineTable();
    inline float fastSine(float phase) const;

    // Optimization: Delay mask (requires power-of-two size)
    int delayMask = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TheMidsRoomAudioProcessor)
};

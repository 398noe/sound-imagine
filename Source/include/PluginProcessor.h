/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "lib/AudioUtilities.h"
#include "lib/SharedAudioData.h"

//==============================================================================
/**
*/
class SoundImagineAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SoundImagineAudioProcessor();
    ~SoundImagineAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void performFFT();
    void pushSample(float sample);


    //==============================================================================
    std::shared_ptr<SharedAudioData> getSharedAudioData();

private:
    juce::dsp::FFT forwardFFT;
    std::shared_ptr<SharedAudioData> shared_audio_data;
    float audio_buffer[SharedAudioData::FFT_SIZE * 2];
    float fft_buffer[SharedAudioData::FFT_SIZE * 2];
    int buffer_index = 0;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundImagineAudioProcessor)
};

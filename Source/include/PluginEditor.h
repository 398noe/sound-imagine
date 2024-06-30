/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "AudioUtilities.h"
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class SoundImagineAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SoundImagineAudioProcessorEditor (SoundImagineAudioProcessor&);
    ~SoundImagineAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SoundImagineAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundImagineAudioProcessorEditor)
};
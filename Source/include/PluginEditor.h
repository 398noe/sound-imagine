/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include "component/Imager.h"
#include "lib/AudioUtilities.h"

//==============================================================================
/**
 */
class SoundImagineAudioProcessorEditor : public juce::AudioProcessorEditor {
  public:
    SoundImagineAudioProcessorEditor(SoundImagineAudioProcessor &);
    ~SoundImagineAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SoundImagineAudioProcessor &audioProcessor;

    // Components
    Imager imager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundImagineAudioProcessorEditor)
};

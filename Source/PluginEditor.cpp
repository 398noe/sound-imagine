/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
SoundImagineAudioProcessorEditor::SoundImagineAudioProcessorEditor(SoundImagineAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p), three_imager(p.getManager()) {
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    // addAndMakeVisible(imager);
    // addAndMakeVisible(lr_imager);
    addAndMakeVisible(three_imager);
    setSize(400, 400);
    setResizable(true, true);
    setResizeLimits(200, 200, 800, 800);
}

SoundImagineAudioProcessorEditor::~SoundImagineAudioProcessorEditor() {}

//==============================================================================
void SoundImagineAudioProcessorEditor::paint(juce::Graphics &g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SoundImagineAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    // imager.setBounds(0, 0, getWidth(), getHeight());
    // lr_imager.setBounds(0, 0, getWidth(), getHeight());
    three_imager.setBounds(0, 0, getWidth(), getHeight());
}

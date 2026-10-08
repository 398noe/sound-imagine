#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

class SoundImagineEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SoundImagineEditor(SoundImagineProcessor&);
    ~SoundImagineEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    juce::Point<float> project(float frequency, float side, float level) const;
    void drawPlot(juce::Graphics&);
    void selectAt(juce::Point<float>);
    SoundImagineProcessor& processor;
    imagine::Snapshot data;
    juce::TextButton threeD { "3D" }, map { "Map" }, freeze { "Freeze" }, help { "?" };
    juce::ComboBox range;
    juce::Rectangle<float> plot;
    int selected = 18;
    bool frozen = false, showHelp = false, stale = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundImagineEditor)
};

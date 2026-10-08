#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

class OverlayButton final : public juce::TextButton
{
public:
    explicit OverlayButton(const juce::String& label) : juce::TextButton(label) {setAlpha(0.4f);}
    void mouseEnter(const juce::MouseEvent& e) override {juce::TextButton::mouseEnter(e); setAlpha(1);}
    void mouseExit(const juce::MouseEvent& e) override {juce::TextButton::mouseExit(e); setAlpha(hasKeyboardFocus(true) ? 1.f : 0.4f);}
    void focusGained(juce::Component::FocusChangeType cause) override {juce::TextButton::focusGained(cause); setAlpha(1);}
    void focusLost(juce::Component::FocusChangeType cause) override {juce::TextButton::focusLost(cause); setAlpha(isMouseOver() ? 1.f : 0.4f);}
};
class SoundImagineEditor final : public juce::AudioProcessorEditor, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit SoundImagineEditor(SoundImagineProcessor&);
    ~SoundImagineEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    juce::Point<float> project(float frequency, float side, float level, const imagine::Camera&, juce::Rectangle<float>) const;
    void drawPlot(juce::Graphics&, const imagine::Camera&, juce::Rectangle<float>, juce::Rectangle<float>);
    juce::Rectangle<float> viewport(int index) const;
    juce::Rectangle<float> plotFor(int index) const;
    imagine::Camera cameraFor(int index) const;
    int viewportAt(juce::Point<float>) const;
    void selectAt(juce::Point<float>);
    void align(int axis);
    void showSettings();
    void updateLanguage();
    juce::String tr(const char* en, const char* ja) const;
    float level(const imagine::Band&) const;
    imagine::Vec3 world(float frequency, float side, float level) const;
    juce::Point<float> screen(imagine::Vec3, const imagine::Camera&, juce::Rectangle<float>) const;
    SoundImagineProcessor& processor;
    imagine::Snapshot data;
    OverlayButton axisX { "X" }, axisY { "Y" }, axisZ { "Z" }, home { "R" }, freeze { "||" }, settings { "..." }, help { "?" };
    juce::TooltipWindow tooltips { this,650 };
    imagine::Camera camera, dragCamera;
    juce::Point<float> dragStart;
    int alignedAxis = -1, lastLanguage = -1;
    bool dragging = false;
    OverlayButton quadButton { "4" };
    juce::Rectangle<float> plot;
    int selected = 18;
    bool frozen = false, showHelp = false, stale = true;
    OverlayButton tableButton { "=" };
    void showTable();
    std::unique_ptr<juce::DocumentWindow> tableWindow;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundImagineEditor)
};

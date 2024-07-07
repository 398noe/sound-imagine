#pragma once

#include <JuceHeader.h>

class GLTest : public juce::Component, public juce::OpenGLRenderer, private juce::Timer {
public:
    GLTest();
    ~GLTest();

    void newOpenGLContextCreated() override;
    void openGLContextClosing() override;
    void renderOpenGL() override;
    void resized() override;
    void timerCallback() override;

    void getDataForPaint();
};

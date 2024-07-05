#include "component/Imager.h"

Imager::Imager(std::shared_ptr<Manager> data) : manager(data)
{
    startTimerHz(60);
}

Imager::~Imager()
{
    stopTimer();
}

void Imager::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.drawRect(getLocalBounds(), 1);
}

void Imager::resized()
{
}

void Imager::timerCallback()
{
    drawFFTData();
}

void Imager::drawFFTData()
{
}

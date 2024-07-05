#pragma once

#include "lib/AudioUtilities.h"
#include "lib/Manager.h"

class Imager : public juce::Component, private juce::Timer
{
public:
    Imager(std::shared_ptr<Manager> data);
    ~Imager() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    void timerCallback() override;

    void drawFFTData();

private:
    bool is_next_fft_block_drawable = true;
    std::shared_ptr<Manager> manager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Imager)
};

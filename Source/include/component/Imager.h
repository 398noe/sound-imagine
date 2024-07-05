#pragma once

#include "lib/AudioUtilities.h"
#include "lib/Manager.h"

class Imager : public juce::Component, private juce::Timer {
  public:
    Imager(std::shared_ptr<Manager> data);
    ~Imager() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    void timerCallback() override;

    void drawFFTData();

  private:
    bool is_next_block_drawable = true;
    std::shared_ptr<Manager> manager;
    std::array<float[FFTConstants::FFT_LENGTH], 2> fft_data;

    float LRMS = 0.0f;
    float RRMS = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Imager)
};

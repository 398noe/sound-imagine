#include "component/Imager.h"

Imager::Imager(std::shared_ptr<Manager> data) : manager(data) { startTimerHz(60); }

Imager::~Imager() { stopTimer(); }

void Imager::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.drawText(juce::String(LRMS), getLocalBounds().removeFromTop(20), juce::Justification::centred);
    g.drawText(juce::String(RRMS), getLocalBounds().removeFromTop(40), juce::Justification::centred);
    g.drawRect(getLocalBounds(), 1);
}

void Imager::resized() { repaint(); }

void Imager::timerCallback() {
    if (this->is_next_block_drawable) {
        this->is_next_block_drawable = false;
        drawFFTData();
        this->is_next_block_drawable = true;
        repaint();
    }
}

void Imager::drawFFTData() {
    // サンプルの値からデシベルを計算
    auto LAudioSample = manager->getAudioSample(0);
    auto RAudioSample = manager->getAudioSample(1);

    // RMSをとる
    this->LRMS = calculateRMS(LAudioSample);
    this->RRMS = calculateRMS(RAudioSample);
}

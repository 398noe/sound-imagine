#include "component/Imager.h"

Imager::Imager(std::shared_ptr<Manager> data) : manager(data) {
    startTimerHz(60);
    setSize(800, 600);
}

Imager::~Imager() { stopTimer(); }

void Imager::paint(juce::Graphics &g) {}

void Imager::resized() { setBounds(0, 0, getWidth(), getHeight()); }

void Imager::timerCallback() {
    if (this->is_next_block_drawable) {
        this->is_next_block_drawable = false;
        getDataForPaint();
        this->is_next_block_drawable = true;
        repaint();
    }
}

void Imager::getDataForPaint() { fft_data = manager->getFFTResult(); }

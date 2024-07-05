#include "component/Imager.h"

Imager::Imager(std::shared_ptr<Manager> data) : manager(data) { startTimerHz(60); }

Imager::~Imager() { stopTimer(); }

void Imager::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::black);

    const int grid_size = 20;
    auto fft_data = manager->getFFTResult(0);
    const int num_squares = fft_data.size();
    const int square_size = getWidth() / grid_size;

    for (int i = 0; i < num_squares; ++i) {
        float value = fft_data[i];
        juce::Colour colour = juce::Colour::fromHSV(0.5f + 0.5f * value, 1.0f, 1.0f, 1.0f);
        int x = (i % grid_size) * square_size;
        int y = (i / grid_size) * square_size;

        g.setColour(colour);
        g.fillRect(x, y, square_size, square_size);
    }
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

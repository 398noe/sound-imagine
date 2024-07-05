#include "component/Imager.h"

Imager::Imager(std::shared_ptr<Manager> data) : manager(data) { startTimerHz(60); }

Imager::~Imager() { stopTimer(); }

void Imager::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::black);

    std::cout << "paint" << std::endl;
    // const int grid_size = 20;
    const int grid_size = 20;
    // const int num_squares = fft_data[0].size();
    // g.setColour(juce::Colours::white);
    // g.drawText("Imager", 0, 0, getWidth(), 20, juce::Justification::centred);
    // g.drawText("num squares: " + std::to_string(num_squares), 0, 20, getWidth(), 20, juce::Justification::centred);
    // const int square_size = getWidth() / grid_size;

    // for (int i = 0; i < num_squares; ++i) {
    //     float value = fft_data[0][i];
    //     juce::Colour colour = juce::Colour::fromHSV(0.5f + 0.5f * value, 1.0f, 1.0f, 1.0f);
    //     int x = (i % grid_size) * square_size;
    //     int y = (i / grid_size) * square_size;

    //     g.setColour(colour);
    //     g.fillRect(x, y, square_size, square_size);
    // }
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
    // LFFT, RFFTからデシベルを計算
    // fft_data[0] = manager->getFFTResult(0);
    // fft_data[1] = manager->getFFTResult(1);
}

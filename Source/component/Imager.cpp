#include "component/Imager.h"

Imager::Imager(std::shared_ptr<Manager> data) : manager(data) {
    startTimerHz(60);
    setSize(800, 600);
}

Imager::~Imager() { stopTimer(); }

void Imager::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::black);
    if (fft_data.empty() || power_spectrum.empty() || energy_difference.empty()) {
        return;
    }

    auto width = getWidth();
    auto height = getHeight();
    auto bin_width = (float)width / FFTConstants::FFT_LENGTH;

    g.setColour(juce::Colours::green);
    // std::cout << (power_spectrum[0][100] + power_spectrum[1][100]) * 0.5f << std::endl;
    // std::cout << (energy_difference[0][100] + energy_difference[1][100]) * 0.5f << std::endl;
    for (size_t i = 0; i < FFTConstants::FFT_LENGTH; i++) { // 下から上に向かってドットを描画する
        auto y = i * bin_width;
        auto left = power_spectrum[0][i];
        auto right = power_spectrum[1][i];
        auto left_x = juce::jmap(left, 0.0f, 100.0f, (float)width / 2.0f, 0.0f);
        auto right_x = juce::jmap(right, 0.0f, 100.0f, (float)width / 2.0f, (float)width);

        juce::Colour bar_color = juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.5f);
        g.setColour(bar_color);
        g.drawLine(left_x, height - y, right_x, height - y, 2.0f);

        auto power = (left + right) * 0.5f;
        juce::Colour color = juce::Colour::fromFloatRGBA(1.0f, 0, 0, juce::jmap(power, 0.0f, 100.0f, 0.5f, 1.0f));

        auto diff = energy_difference[0][i];
        auto offset = juce::jmap(diff, -100.0f, 100.0f, -0.5f, 0.5f);

        auto dotX = (1 - 2.0f * offset) * (width / 2.0f);
        auto dotY = height - y;

        g.setColour(color);
        g.fillEllipse(dotX, dotY, 4.0f, 4.0f);
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
    manager->calculatePowerSpectrum();
    manager->calculatePowerSpectrum();
    fft_data = manager->getFFTResult();
    power_spectrum = manager->getPowerSpectrum();
    energy_difference = manager->getEnergyDifference();
}

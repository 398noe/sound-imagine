#include "component/LRImager.h"

LRImager::LRImager(std::shared_ptr<Manager> data) : manager(data) { startTimerHz(60); }

LRImager::~LRImager() { stopTimer(); }

void LRImager::paint(juce::Graphics &g) {
    auto fft_freqs = manager->getFFTFreqs();
    fft_freqs[0] = 10.0f;
    auto width = getWidth();
    auto height = getHeight();
    g.fillAll(juce::Colours::black);

    // LRのPower Spectrumを描画
    g.setColour(juce::Colours::green);
    for (size_t i = 0; i < FFTConstants::FFT_LENGTH; i++) {
        auto freq = fft_freqs[i];
        auto log = std::log10(freq);
        auto x = juce::jmap(log, 1.0f, 5.0f, 1.0f, 5.0f);
        // 左は20Hz, 右は20kHzになるように描画
        auto dot_x = juce::jmap(x, 1.0f, 5.0f, 0.0f, (float)width);
        auto left = power_spectrum[0][i];
        auto right = power_spectrum[1][i];
        auto left_y = juce::jmap(left, 0.0f, 100.0f, (float)height, 0.0f);
        auto right_y = juce::jmap(right, 0.0f, 100.0f, (float)height, 0.0f);
        g.drawLine(dot_x, left_y, dot_x, right_y, 2.0f);
    }
};

void LRImager::resized() { setBounds(0, 0, getWidth(), getHeight()); };

void LRImager::timerCallback() {
    if (is_next_block_drawable) {
        is_next_block_drawable = false;
        draw();
        repaint();
        is_next_block_drawable = true;
    }
};

void LRImager::draw() {
    manager->calculatePowerSpectrum();
    manager->calculateEnergyDifference();
    fft_data = manager->getFFTResult();
    power_spectrum = manager->getPowerSpectrum();
    energy_difference = manager->getEnergyDifference();
};
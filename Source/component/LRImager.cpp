#include "component/LRImager.h"

LRImager::LRImager(std::shared_ptr<Manager> data) : manager(data) { startTimerHz(60); }

LRImager::~LRImager() { stopTimer(); }

void LRImager::paint(juce::Graphics &g) {
    if (fft_data.empty() || power_spectrum.empty() || energy_difference.empty()) {
        return;
    }
    auto fft_freqs = manager->getFFTFreqs();
    fft_freqs[0] = 10.0f;
    auto width = getWidth();
    auto height = getHeight();
    auto center_x = width / 2.0f;

    g.fillAll(juce::Colours::black);
    // LRのPower Spectrumを描画
    for (size_t i = 0; i < FFTConstants::FFT_LENGTH; i++) {
        auto freq = fft_freqs[i];
        auto log = std::log10(freq);
        auto left = power_spectrum[0][i];
        auto right = power_spectrum[1][i];
        auto diff = energy_difference[0][i];
        auto left_x = juce::jmap(left, 0.0f, 100.0f, center_x, 0.0f);
        auto right_x = juce::jmap(right, 0.0f, 100.0f, center_x, float(width));
        // 一番下が20Hz, 一番上が20kHzになるようにyの座標を決める。
        // fft_freqsがそれ以外の範囲を持つ場合があるが, その場合はウィンドウの外に描画する
        auto y = height - (log - 1.30103f) / 3.0f * height;
        g.setColour(juce::Colours::green);
        g.drawLine(left_x, y, right_x, y, 2.0f);

        auto power = (left + right) * 0.5f;
        auto color = juce::Colour::fromFloatRGBA(1.0f, 0.0f, 0.0f, juce::jmap(power, 0.0f, 10.0f, 0.5f, 1.0f));
        auto offset = juce::jmap(diff, -50.0f, 50.0f, -0.5f, 0.5f);
        auto dot_x = (1 + 2.0f * offset) * center_x;
        g.setColour(color);
        g.fillEllipse(dot_x, y, 4.0f, 4.0f);

        
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
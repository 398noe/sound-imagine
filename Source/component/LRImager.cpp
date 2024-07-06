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
    // 中心に線を引く
    g.setColour(juce::Colours::white);
    g.drawLine(center_x, 0, center_x, height, 1.0f);

    // 左右にLRの文字を書く
    g.setFont(16.0f);
    g.drawText("L", 0, 0, 20, 20, juce::Justification::centred);
    g.drawText("R", width - 20, 0, 20, 20, juce::Justification::centred);
    // 中心から左右に上下方向に線を引く
    g.setColour(juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.5f));
    g.drawLine(width >> 2, 0, width >> 2, height, 1.0f);
    g.drawLine(width * 0.75f, 0, width * 0.75f, height, 1.0f);

    // 31.25Hz, 62.5Hz, 125Hz, 250Hz, 500Hz, 1kHz, 2kHz, 4kHz, 8kHz, 16kHzに線を引く
    for (int i = 0; i < 10; i++) {
        auto hz = 31.25f * std::pow(2.0f, i);
        auto log = std::log10(hz);
        auto y = height - (log - 1.30103f) / 3.0f * height;
        g.setColour(juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.5f));
        g.drawLine(0, y, width, y, 1.0f);
        g.setFont(10.0f);
        g.drawText(std::to_string(static_cast<int>(hz)), 0, y, 50, 20, juce::Justification::left);
    }

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
        g.setColour(juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.2f));
        g.drawLine(left_x, y, right_x, y, 2.0f);

        auto power = (left + right) * 0.5f;
        auto color = juce::Colour::fromFloatRGBA(0.0f, 1.0f, 0.0f, juce::jmap(power, 0.0f, 10.0f, 0.5f, 1.0f));
        auto offset = juce::jmap(diff, -50.0f, 50.0f, -0.5f, 0.5f);
        auto dot_x = (1 + 2.0f * offset) * center_x;
        g.setColour(color);
        g.fillEllipse(dot_x - 2.0f, y - 2.0f, 4.0f, 4.0f);
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
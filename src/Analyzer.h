#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

namespace imagine
{
constexpr int fftOrder = 13, fftSize = 1 << fftOrder, hopSize = 2048, bandCount = 32;
struct Band
{
    float low = 0, high = 0, levelDb = -90, side = 0, correlation = 0, balance = 0;
    bool active = false, correlationValid = false;
};
struct Snapshot
{
    std::array<Band, bandCount> bands {};
    double sampleRate = 48000;
    std::uint64_t frames = 0;
    double capturedMs = 0;
};
class Analyzer
{
public:
    Analyzer();
    void reset(double sampleRate);
    bool push(float left, float right);
    const Snapshot& snapshot() const noexcept { return result; }
private:
    void analyse();
    juce::dsp::FFT fft { fftOrder };
    std::array<float, fftSize> window {}, ringL {}, ringR {};
    std::array<float, 2 * fftSize> spectrumL {}, spectrumR {};
    struct Power { double left = 0, right = 0, cross = 0; };
    std::array<Power, bandCount> powers {};
    Snapshot result;
    int position = 0, collected = 0, hop = 0;
    double windowEnergy = 0, retention = 0;
};
}

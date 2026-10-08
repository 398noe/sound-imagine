#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>
#include <vector>

namespace imagine
{
constexpr int fftOrder = 13, fftSize = 1 << fftOrder, hopSize = 2048, bandCount = 32;
constexpr int maxBands = 128;
struct Band
{
    float low = 0, high = 0, levelDb = -90, side = 0, correlation = 0, balance = 0;
    bool active = false, correlationValid = false;
};
struct Snapshot
{
    std::array<Band, maxBands> bands {};
    int numBands = bandCount;
    double sampleRate = 48000;
    std::uint64_t frames = 0;
    double capturedMs = 0;
    int fftPoints = fftSize;
    int averagingMs = 250;
};
class Analyzer
{
public:
    Analyzer();
    void reset(double sampleRate, int order = fftOrder, int smoothingMs = 250, int bands = bandCount);
    bool push(float left, float right);
    const Snapshot& snapshot() const noexcept { return result; }
private:
    void analyse();
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window, ringL, ringR, spectrumL, spectrumR;
    int size = fftSize, step = hopSize, currentOrder = 0;
    struct Power { double left = 0, right = 0, cross = 0; };
    std::array<Power, maxBands> powers {};
    Snapshot result;
    int position = 0, collected = 0, hop = 0;
    double windowEnergy = 0, retention = 0;
};
}

#pragma once

#include "lib/Processor.h"

class Manager {
  public:
    Manager();
    ~Manager();

    void addAudioSample(float sample, int channel);
    void addAudioSampleOnce(float left, float right);
    void calculateFFT();
    void setFFTResult();
    void setSampleRate(double sample_rate);
    double getSampleRate();
    std::array<float[FFTConstants::FFT_LENGTH], 4> getFFTResult();
    std::array<float, FFTConstants::FFT_LENGTH> getFFTFreqs();

  private:
    Processor processor[4];
    double sample_rate = 0.0;
    std::array<float[FFTConstants::FFT_LENGTH], 4> fft_result = {0.0f}; // 0: left, 1: right, 2: mid, 3: side
    std::array<float, FFTConstants::FFT_LENGTH> fft_freqs = {0.0f};
};
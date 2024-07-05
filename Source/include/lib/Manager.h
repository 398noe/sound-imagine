#pragma once

#include "lib/Processor.h"

class Manager {
  public:
    Manager();
    ~Manager();

    void addAudioSample(float sample, int channel);
    void addAudioSampleOnce(float left, float right);
    void calculateFFT();
    void calculatePowerSpectrum();
    void calculateEnergyDifference();
    void setFFTResult();
    std::array<float[FFTConstants::FFT_LENGTH], 4> getFFTResult();
    std::array<float[FFTConstants::FFT_LENGTH], 4> getPowerSpectrum();
    std::array<float[FFTConstants::FFT_LENGTH], 2> getEnergyDifference();

    int getBufferIdx(int channel);

    void inferLR();

  private:
    Processor processor[4];
    std::array<float[FFTConstants::FFT_LENGTH], 4> fft_result; // 0: left, 1: right, 2: mid, 3: side
    std::array<float[FFTConstants::FFT_LENGTH], 4> power_spectrum; // 0: left, 1: right, 2: mid, 3: side
    std::array<float[FFTConstants::FFT_LENGTH], 2> energy_difference; // 0: LR, 1: MS
};
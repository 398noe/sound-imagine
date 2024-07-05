#pragma once

#include "lib/Processor.h"

class Manager {
  public:
    Manager();
    ~Manager();

    void addAudioSample(float sample, int channel);
    void calculateFFT();
    void calculatePowerSpectrum();
    void setFFTResult();
    std::array<float[FFTConstants::FFT_LENGTH], 2> getFFTResult();
    std::array<float[FFTConstants::FFT_LENGTH], 2> getPowerSpectrum();
    float* getFFTResult(int channel);
    int getBufferIdx(int channel);

    void inferLR();

    bool isAudioBufferReady();

  private:
    Processor processor[2];
    std::array<float[FFTConstants::FFT_LENGTH], 2> fft_result;
    std::array<float[FFTConstants::FFT_LENGTH], 2> power_spectrum;
};
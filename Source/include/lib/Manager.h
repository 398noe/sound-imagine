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
    void setFFTResult();
    std::array<float[FFTConstants::FFT_LENGTH], 4> getFFTResult();
    std::array<float[FFTConstants::FFT_LENGTH], 4> getPowerSpectrum();
    float* getFFTResult(int channel);
    int getBufferIdx(int channel);

    void inferLR();

  private:
    Processor processor[4];
    std::array<float[FFTConstants::FFT_LENGTH], 4> fft_result;
    std::array<float[FFTConstants::FFT_LENGTH], 4> power_spectrum;
};
#pragma once

#include "lib/Processor.h"

class Manager {
  public:
    Manager();
    ~Manager();

    void addAudioSample(float sample, int channel);
    void calculateFFT();
    void setFFTResult();
    float* getFFTResult(int channel);
    int getBufferIdx(int channel);

    void inferLR();

    bool isAudioBufferReady();

  private:
    Processor processor[2];
    std::array<float[(FFTConstants::FFT_SIZE >> 1) + 1], 2> fft_result;
};
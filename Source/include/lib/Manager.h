#pragma once

#include "lib/Processor.h"

class Manager {
  public:
    Manager();
    ~Manager();

    void addAudioSample(float sample, int channel);
    void calculateFFT();
    void setFFTResult();
    float *getAudioSample(int channel);
    float *getFFTResult(int channel);

    void inferLR();

    bool isAudioBufferReady();

  private:
    Processor processor[2];
    float fft_result[2][FFTConstants::FFT_SIZE];
};
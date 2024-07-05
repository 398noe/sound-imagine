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
    std::vector<float> getFFTResult(int channel);

    void inferLR();

    bool isAudioBufferReady();

  private:
    Processor processor[2];
    std::array<std::vector<float>, 2> fft_result;
};
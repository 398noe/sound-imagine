#pragma once

#include "AudioUtilities.h"
#include "Constant.h"
#include <cstdint>

class Manager;

class Processor {
  public:
    Processor();
    ~Processor();

    void doFFT();
    float *getFFTResult();

    void addAudioSample(float sample);

    uint16_t getBufferIdx();

  private:
    juce::dsp::FFT forwardFFT;
    float fft_result[FFTConstants::FFT_SIZE] = {0.0f};
    float buffer[FFTConstants::FFT_SIZE] = {0.0f};
    uint16_t buffer_idx = 0;
};

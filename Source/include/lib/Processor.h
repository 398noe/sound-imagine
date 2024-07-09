#pragma once

#include "AudioUtilities.h"
#include "Constant.h"
#include <complex>
#include <cstdint>

class Manager;

class Processor {
  public:
    Processor();
    ~Processor();

    void doFFTMagnitude();
    void doFFTReal();
    float *getFFTResultMagnitude();
    float *getFFTResult();

    void addAudioSample(float sample);

    uint16_t getBufferIdx();

  private:
    juce::dsp::FFT forwardFFT;
    float fft_result_magnitude[FFTConstants::FFT_SIZE] = {0.0f};
    float fft_result[FFTConstants::FFT_SIZE << 1] = {0.0f};
    float buffer[FFTConstants::FFT_SIZE] = {0.0f};
    uint16_t buffer_idx = 0;
};

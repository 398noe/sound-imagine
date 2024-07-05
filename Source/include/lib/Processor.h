#pragma once

#include "AudioUtilities.h"
#include "Constant.h"
#include <cstdint>

class Manager;

class Processor
{
public:
    Processor();
    ~Processor();

    void doFFT();
    float* getFFTResult();

    float* cloneAudioSample(); // clone buffer
    float* getAudioSample(uint16_t start); // get buffer in chronological order
    void addAudioSample(float sample); // add sample to buffer

    uint16_t getBufferIdx();

private:
    juce::dsp::FFT forwardFFT;
    float fft_result[FFTConstants::FFT_SIZE];
    float buffer[FFTConstants::FFT_SIZE];
    uint16_t buffer_idx = 0;
};

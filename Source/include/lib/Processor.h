#pragma once

#include "AudioUtilities.h"
#include "SharedAudioData.h"
#include <cstdint>

class Processor
{
public:
    Processor();
    ~Processor();

    void doFFT();

    float* cloneAudioSample(); // clone buffer
    float* getAudioSample(uint16_t start); // get buffer in chronological order
    void addAudioSample(float sample); // add sample to buffer

private:
    juce::dsp::FFT forwardFFT;
    float buffer[SharedAudioData::FFT_SIZE];
    uint16_t buffer_idx = 0;
};

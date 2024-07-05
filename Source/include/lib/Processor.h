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

    float* getAudioSample(uint16_t offset);
    void addAudioSample(float sample);

private:
    juce::dsp::FFT forwardFFT;
    float buffer[SharedAudioData::FFT_SIZE];
    uint16_t buffer_idx = 0;
};

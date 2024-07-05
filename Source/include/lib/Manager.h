#pragma once

#include "processor.h"

class Manager {
public:
    static const int FFT_ORDER = 10;
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;
    static constexpr int FFT_MASK = FFT_SIZE - 1;

    Manager();
    ~Manager();

    void addAudioSample(float sample, int channel);
    void calculateFFT();

    void inferLR();

    bool isAudioBufferReady();

private:
    Processor processor[2]; // 0 is for left channel, 1 is for right
};
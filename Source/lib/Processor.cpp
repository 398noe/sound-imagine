#include "lib/Processor.h"
#include "lib/Constant.h"

Processor::Processor() : forwardFFT(FFTConstants::FFT_ORDER) {}

Processor::~Processor() {}

void Processor::doFFT() {
    for (int i = 0; i < FFTConstants::FFT_SIZE; i++) {
        fft_result[i] = buffer[(buffer_idx + i) & FFTConstants::FFT_MASK];
    }
    forwardFFT.performFrequencyOnlyForwardTransform(fft_result);
}

float *Processor::getFFTResult() { return fft_result; }

void Processor::addAudioSample(float sample) {
    buffer[buffer_idx++] = sample;
    buffer_idx &= FFTConstants::FFT_MASK;
}

uint16_t Processor::getBufferIdx() { return buffer_idx; }
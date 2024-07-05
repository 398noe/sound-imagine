#include "lib/Processor.h"

Processor::Processor() : forwardFFT(Manager::FFT_ORDER)
{
}

Processor::~Processor()
{
}

void Processor::doFFT()
{
    float* block = getAudioSample(buffer_idx);
    forwardFFT.performRealOnlyForwardTransform(block);
    // block means fft result data
    for (int i = 0; i < Manager::FFT_SIZE; i++)
    {
        fft_result[i] = block[i];
    }
}

float* Processor::getFFTResult()
{
    return fft_result;
}

float* Processor::cloneAudioSample() {
    float block[Manager::FFT_SIZE];
    juce::zeromem(block, sizeof(block));
    for (int i = 0; i < Manager::FFT_SIZE; i++)
    {
        block[i] = buffer[i];
    }
    return block;
}

float* Processor::getAudioSample(uint16_t start = 0)
{
    float block[Manager::FFT_SIZE];
    juce::zeromem(block, sizeof(buffer));
    for (int i = 0; i < Manager::FFT_SIZE; i++)
    {
        block[i] = buffer[(start + i) & Manager::FFT_MASK];
    }
    return block;
}

void Processor::addAudioSample(float sample)
{
    buffer[buffer_idx++] = sample;
    buffer_idx &= Manager::FFT_MASK;
}

uint16_t Processor::getBufferIdx()
{
    return buffer_idx;
}
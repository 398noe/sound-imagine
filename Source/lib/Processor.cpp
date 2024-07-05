#include "Processor.h"

Processor::Processor() : forwardFFT(SharedAudioData::FFT_ORDER)
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
}

float* Processor::cloneAudioSample() {
    float block[SharedAudioData::FFT_SIZE];
    juce::zeromem(block, sizeof(block));
    for (int i = 0; i < SharedAudioData::FFT_SIZE; i++)
    {
        block[i] = buffer[i];
    }
    return block;
}

float* Processor::getAudioSample(uint16_t start = 0)
{
    float block[SharedAudioData::FFT_SIZE];
    juce::zeromem(block, sizeof(buffer));
    for (int i = 0; i < SharedAudioData::FFT_SIZE; i++)
    {
        block[i] = buffer[(start + i) & SharedAudioData::FFT_MASK];
    }
    return block;
}

void Processor::addAudioSample(float sample)
{
    buffer[buffer_idx++] = sample;
    buffer_idx &= SharedAudioData::FFT_MASK;
}
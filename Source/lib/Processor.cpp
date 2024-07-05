#include "Processor.h"

Processor::Processor() : forwardFFT(SharedAudioData::FFT_ORDER)
{
}

Processor::~Processor()
{
}

float* Processor::getAudioSample(uint16_t offset)
{
    return &buffer[(buffer_idx + offset) & SharedAudioData::FFT_MASK];

}

void Processor::addAudioSample(float sample)
{
    buffer[buffer_idx++] = sample;
    buffer_idx &= SharedAudioData::FFT_MASK;
}
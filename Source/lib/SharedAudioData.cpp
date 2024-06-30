#include "lib/SharedAudioData.h"

bool SharedAudioData::is_next_fft_block_ready = false;

SharedAudioData::SharedAudioData()
{
}

SharedAudioData::~SharedAudioData()
{
}

float *SharedAudioData::getFFTData()
{
    return fft_data;
}

void SharedAudioData::setFFTData(float *data)
{
    for (int i = 0; i < FFT_SIZE * 2; i++)
    {
        fft_data[i] = data[i];
    }
}

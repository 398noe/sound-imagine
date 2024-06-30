#pragma once

class SharedAudioData
{
public:
    static const int FFT_ORDER = 10;
    static const int FFT_SIZE = 1 << FFT_ORDER;

    static bool is_next_fft_block_ready;
    SharedAudioData();
    ~SharedAudioData();

    float *getFFTData();
    void setFFTData(float *data);

private:
    float fft_data[FFT_SIZE * 2];
};
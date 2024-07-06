#include "lib/Manager.h"

Manager::Manager() : processor() {}

Manager::~Manager() {}

void Manager::calculateFFT() {
    for (int i = 0; i < 4; i++) {
        processor[i].doFFT();
    }
}

void Manager::addAudioSampleOnce(float left, float right) {
    processor[0].addAudioSample(left);               // left
    processor[1].addAudioSample(right);              // right
    processor[2].addAudioSample((left + right) / 2); // mid
    processor[3].addAudioSample((left - right) / 2); // side
}

void Manager::addAudioSample(float sample, int channel) { processor[channel].addAudioSample(sample); }

void Manager::setFFTResult() {
    for (int i = 0; i < 4; i++) {
        float *fft = processor[i].getFFTResult();

        for (int j = 0; j < FFTConstants::FFT_LENGTH; j++) {
            this->fft_result[i][j] = fft[j];
        }
    }
}

void Manager::setSampleRate(double sr) {
    this->sample_rate = sr;
    for (int i = 0; i < FFTConstants::FFT_LENGTH; i++) {
        fft_freqs[i] = static_cast<float>(i) * static_cast<float>(sr) / static_cast<float>(FFTConstants::FFT_SIZE);
    }
    fft_freqs[0] = 0.0f;
}

double Manager::getSampleRate() { return this->sample_rate; }

std::array<float[FFTConstants::FFT_LENGTH], 4> Manager::getFFTResult() { return fft_result; }
std::array<float, FFTConstants::FFT_LENGTH> Manager::getFFTFreqs() { return fft_freqs; }

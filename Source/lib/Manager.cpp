#include "lib/Manager.h"

Manager::Manager() : processor() {}

Manager::~Manager() {}

void Manager::calculateFFT() {
    for (int i = 0; i < 4; i++) {
        processor[i].doFFT();
    }
}

void Manager::calculatePowerSpectrum() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < FFTConstants::FFT_LENGTH; j++) {
            this->power_spectrum[i][j] = fft_result[i][j] * fft_result[i][j];
        }
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

std::array<float[FFTConstants::FFT_LENGTH], 4> Manager::getFFTResult() { return fft_result; }
std::array<float[FFTConstants::FFT_LENGTH], 4> Manager::getPowerSpectrum() { return power_spectrum; }

float *Manager::getFFTResult(int channel) { return fft_result[channel]; }

void Manager::inferLR() {
    float *left = fft_result[0];
    float *right = fft_result[1];

    for (int i = 0; i < FFTConstants::FFT_LENGTH; i++) {
        float l = left[i];
        float r = right[i];

        left[i] = (l + r) / 2;
        right[i] = (l - r) / 2;
    }
}

int Manager::getBufferIdx(int channel) { return processor[channel].getBufferIdx(); }

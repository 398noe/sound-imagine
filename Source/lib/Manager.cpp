#include "lib/Manager.h"

Manager::Manager() : processor() {}

Manager::~Manager() {}

void Manager::calculateFFT() {
    for (int i = 0; i < 2; i++) {
        processor[i].doFFT();
    }
}

void Manager::calculatePowerSpectrum() {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < FFTConstants::FFT_LENGTH; j++) {
            this->power_spectrum[i][j] = fft_result[i][j] * fft_result[i][j];
        }
    }
}

void Manager::calculateMidSide() {
    for (int i = 0; i < FFTConstants::FFT_LENGTH; i++) {
        float l = fft_result[0][i];
        float r = fft_result[1][i];

        mid_side[0][i] = (l + r) / 2;
        mid_side[1][i] = (l - r) / 2;
    }
}

void Manager::addAudioSample(float sample, int channel) { processor[channel].addAudioSample(sample); }

void Manager::setFFTResult() {
    for (int i = 0; i < 2; i++) {
        float *fft = processor[i].getFFTResult();

        for (int j = 0; j < FFTConstants::FFT_LENGTH; j++) {
            this->fft_result[i][j] = fft[j];
        }
    }
}

std::array<float[FFTConstants::FFT_LENGTH], 2> Manager::getFFTResult() { return fft_result; }
std::array<float[FFTConstants::FFT_LENGTH], 2> Manager::getPowerSpectrum() { return power_spectrum; }
std::array<float[FFTConstants::FFT_LENGTH], 2> Manager::getMidSide() { return mid_side; }

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

bool Manager::isAudioBufferReady() {
    return processor[0].getBufferIdx() == FFTConstants::FFT_MASK && processor[1].getBufferIdx() == FFTConstants::FFT_MASK;
}

int Manager::getBufferIdx(int channel) { return processor[channel].getBufferIdx(); }
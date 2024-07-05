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

void Manager::calculateEnergyDifference() {
    for (int i = 0; i < FFTConstants::FFT_LENGTH; i++) {
        this->energy_difference[0][i] = power_spectrum[1][i] - power_spectrum[0][i]; // LR
        this->energy_difference[1][i] = power_spectrum[3][i] - power_spectrum[2][i]; // MS
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
std::array<float[FFTConstants::FFT_LENGTH], 2> Manager::getEnergyDifference() { return energy_difference; }

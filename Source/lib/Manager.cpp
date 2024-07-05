#include "lib/Manager.h"
#include "Manager.h"

Manager::Manager() : processor() {}

Manager::~Manager() {}

void Manager::calculateFFT() {
    for (int i = 0; i < 2; i++) {
        processor[i].doFFT();
    }
}

void Manager::addAudioSample(float sample, int channel) { processor[channel].addAudioSample(sample); }

void Manager::setFFTResult() {
    for (int i = 0; i < 2; i++) {
        float *fft = processor[i].getFFTResult();
        for (int j = 0; j < FFTConstants::FFT_SIZE; j++) {
            this->fft_result[i][j] = fft[j];
        }
    }
}

float *Manager::getAudioSample(int channel) {
    auto start = processor[channel].getBufferIdx();
    return processor[channel].getAudioSample(start);
}

float *Manager::getFFTResult(int channel) { return fft_result[channel]; }

void Manager::inferLR() {}

bool Manager::isAudioBufferReady() {
    return processor[0].getBufferIdx() == FFTConstants::FFT_SIZE && processor[1].getBufferIdx() == FFTConstants::FFT_SIZE;
}
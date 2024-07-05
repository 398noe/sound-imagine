#include "lib/Manager.h"

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

        for (int j = 0; j < FFTConstants::FFT_LENGTH; j++) {
            this->fft_result[i][j] = fft[j];
        }
    }
}

float *Manager::getFFTResult(int channel) { return fft_result[channel]; }

void Manager::inferLR() {}

bool Manager::isAudioBufferReady() {
    return processor[0].getBufferIdx() == FFTConstants::FFT_MASK && processor[1].getBufferIdx() == FFTConstants::FFT_MASK;
}

int Manager::getBufferIdx(int channel) { return processor[channel].getBufferIdx(); }
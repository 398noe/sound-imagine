#include "lib/Processor.h"
#include "lib/Constant.h"

Processor::Processor() : forwardFFT(FFTConstants::FFT_ORDER) {}

Processor::~Processor() {}

void Processor::doFFT() {
    for (int i = 0; i < FFTConstants::FFT_SIZE; i++) {
        fft_result[i] = buffer[(buffer_idx + i) & FFTConstants::FFT_MASK];
    }
    forwardFFT.performFrequencyOnlyForwardTransform(fft_result);
}

void Processor::doCQT(const float sr, const float min, const int bins, const int octs) {
    for (int i = 0; i < FFTConstants::FFT_SIZE; i++) {
        fft_result[i] = buffer[(buffer_idx + i) & FFTConstants::FFT_MASK];
    }
    std::vector<float> cqt_input;
    for (int i = 0; i < FFTConstants::FFT_SIZE; i++) {
        cqt_input.push_back(fft_result[i]);
    }
    auto center_freqs = calculateCenterFreqs(min, bins, octs);
    auto freq_max = center_freqs[center_freqs.size() - 1];
    auto fft_sizes = calculateFFTSize(center_freqs, sr);

    auto cqt_result = calculateCQT(min, bins, octs, cqt_input, sr);
    // cqt_resultの絶対値を取って、fft_resultに格納する
    auto size = cqt_result.size();
    for (int i = 0; i < cqt_result.size(); i++) {
        fft_result[i] = std::abs(cqt_result[i][0]);
    }
}

float *Processor::getFFTResult() { return fft_result; }

void Processor::addAudioSample(float sample) {
    buffer[buffer_idx++] = sample;
    buffer_idx &= FFTConstants::FFT_MASK;
}

uint16_t Processor::getBufferIdx() { return buffer_idx; }
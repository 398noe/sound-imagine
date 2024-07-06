#include "lib/AudioUtilities.h"
#include "lib/Constant.h"
#define PI 3.141592653589793

float calculateRMS(float *buffer) {
    float sum = 0;
    for (int i = 0; i < FFTConstants::FFT_SIZE; i++) {
        sum += buffer[i] * buffer[i];
    }
    return sqrt(sum / FFTConstants::FFT_SIZE);
}

std::vector<float> calculateCenterFreqs(float min, int bins, int octs) {
    std::vector<float> freqs;
    for (int i = 0; i < bins * octs; ++i) {
        float f = min * std::pow(2.0f, static_cast<float>(i) / bins);
        freqs.push_back(f);
    }
    return freqs;
}

std::vector<float> calculateFFTSize(std::vector<float> center_freqs, float sr) {
    std::vector<float> sizes;
    for (int i = 0; i < center_freqs.size(); ++i) {
        int size = static_cast<int>(sr / center_freqs[i]);
        if (size % 2 == 0) {
            size++;
        }
        sizes.push_back(size);
    }
    return sizes;
}

std::vector<float> generateHannWindow(int size) {
    std::vector<float> window;
    for (int i = 0; i < size; ++i) {
        window.push_back(0.5f * (1.0f - std::cos(2.0f * PI * i / (size - 1))));
    }
    return window;
}

void performFFT(const std::vector<float> &input, std::vector<std::complex<double>> &output) {
    int N = input.size();
    juce::dsp::FFT fft(std::log2(N));

    juce::HeapBlock<juce::dsp::Complex<float>> fftData(N * 2);
    for (int i = 0; i < N; ++i) {
        fftData[i].real(input[i]);
        fftData[i].imag(0.0f);
    }

    fft.perform(fftData, fftData, false);

    output.resize(N);
    for (int i = 0; i < N; ++i) {
        output[i] = std::complex<double>(fftData[i].real(), fftData[i].imag());
    }
}
std::vector<std::vector<std::complex<double>>> calculateCQT(float min, int bins, int octs, const std::vector<float> &sample, float sr) {
    auto center_freqs = calculateCenterFreqs(min, bins, octs);
    auto freq_max = center_freqs[center_freqs.size() - 1];
    // CQTに使うFFTのサイズを計算
    auto fft_sizes = calculateFFTSize(center_freqs, sr);

    auto nyquist = sr / 2.0f;

    // CQTの計算を行う
    std::vector<std::vector<std::complex<double>>> cqt;

    for (int i = 0; i < center_freqs.size(); ++i) {
        int win_size = static_cast<int>(sr / center_freqs[i] * 2);
        std::vector<float> hann_window = generateHannWindow(win_size);

        std::vector<float> fft_input;
        for (int j = 0; j < win_size; ++j) {
            if (i < sample.size()) {
                fft_input[i] = sample[i] * hann_window[i];
            } else {
                fft_input[i] = 0.0f;
            }
        }
        std::vector<std::complex<double>> fft_result;
        performFFT(fft_input, fft_result);
        cqt.push_back(fft_result);
    }

    return cqt;
}
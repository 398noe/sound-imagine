#pragma once

#include "../../JuceLibraryCode/JucePluginDefines.h"
#include <JuceHeader.h>
#include <complex>

float calculateRMS(float *buffer);

// default min is 32.703Hz, which is C1
std::vector<float> calculateCenterFreqs(float min, int bins, int octs);

std::vector<float> calculateFFTSize(std::vector<float> center_freqs, float sr);

std::vector<float> generateHannWindow(int size);

void performFFT(const std::vector<float> &input, std::vector<std::complex<double>> &output);

std::vector<std::vector<std::complex<double>>> calculateCQT(float min, int bins, int octs, const std::vector<float> &sample, float sr);

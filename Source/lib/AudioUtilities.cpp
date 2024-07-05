#include "lib/AudioUtilities.h"
#include "lib/Constant.h"

float calculateRMS(float *buffer) {
    float sum = 0;
    for (int i = 0; i < FFTConstants::FFT_SIZE; i++) {
        sum += buffer[i] * buffer[i];
    }
    return sqrt(sum / FFTConstants::FFT_SIZE);
}

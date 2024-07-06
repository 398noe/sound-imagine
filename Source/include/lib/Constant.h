#pragma once

namespace FFTConstants {
const int FFT_ORDER = 10;
const int HOP_LENGTH = 1024;
constexpr int FFT_SIZE = 1 << FFT_ORDER;
constexpr int FFT_MASK = FFT_SIZE - 1;
constexpr int FFT_LENGTH = (FFT_SIZE >> 1) + 1;
} // namespace FFTConstants
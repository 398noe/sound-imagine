#pragma once

namespace FFTConstants {
const int FFT_ORDER = 10;
constexpr int FFT_SIZE = 1 << FFT_ORDER;
constexpr int FFT_MASK = FFT_SIZE - 1;
} // namespace FFTConstants
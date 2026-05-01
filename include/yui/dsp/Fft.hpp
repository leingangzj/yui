#pragma once
// Minimal radix-2 in-place Cooley-Tukey FFT. No dependencies, freestanding.
// `n` must be a power of two. Input is split-format real/imag arrays.
#include <cmath>
#include <cstddef>
#include <utility>

namespace yui::dsp {

inline void fft_radix2(float* re, float* im, std::size_t n) {
  for (std::size_t i = 1, j = 0; i < n; ++i) {
    std::size_t bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j |= bit;
    if (i < j) {
      std::swap(re[i], re[j]);
      std::swap(im[i], im[j]);
    }
  }
  constexpr float kPi = 3.14159265358979323846f;
  for (std::size_t len = 2; len <= n; len <<= 1) {
    const float theta = -2.f * kPi / static_cast<float>(len);
    const float wn_r  = std::cos(theta);
    const float wn_i  = std::sin(theta);
    const std::size_t half = len >> 1;
    for (std::size_t i = 0; i < n; i += len) {
      float w_r = 1.f, w_i = 0.f;
      for (std::size_t k = 0; k < half; ++k) {
        const float a_r = re[i + k];
        const float a_i = im[i + k];
        const float b_r = re[i + k + half];
        const float b_i = im[i + k + half];
        const float t_r = w_r * b_r - w_i * b_i;
        const float t_i = w_r * b_i + w_i * b_r;
        re[i + k]        = a_r + t_r;
        im[i + k]        = a_i + t_i;
        re[i + k + half] = a_r - t_r;
        im[i + k + half] = a_i - t_i;
        const float nw_r = w_r * wn_r - w_i * wn_i;
        const float nw_i = w_r * wn_i + w_i * wn_r;
        w_r = nw_r;
        w_i = nw_i;
      }
    }
  }
}

// Hann window, in-place on real signal.
inline void hann_window(float* x, std::size_t n) {
  constexpr float kPi = 3.14159265358979323846f;
  for (std::size_t i = 0; i < n; ++i) {
    const float w = 0.5f * (1.f - std::cos(2.f * kPi * static_cast<float>(i) /
                                            static_cast<float>(n - 1)));
    x[i] *= w;
  }
}

}  // namespace yui::dsp

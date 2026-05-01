#pragma once
// VU meter + 8-bar log-spaced spectrum visualizer. Pulls a frame of
// samples each tick, computes RMS for the VU meter and a Hann-windowed
// 256-pt FFT for the spectrum bands. Assumes 16 kHz sample rate (see
// Esp32Mic::kSampleRate); bins are log-grouped to roughly span 60 Hz – 8 kHz.
#include "yui/app/App.hpp"
#include "yui/dsp/Fft.hpp"
#include "yui/hal/IMic.hpp"
#include "yui/types.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace yui {

class MicApp : public App {
public:
  static constexpr size_t kFrame = 256;
  static constexpr int    kBands = 8;

  explicit MicApp(IMic& mic) : mic_(mic) {}
  const char* name() const override { return "Mic Visualizer"; }

  void on_enter(Hal& /*hal*/) override {
    mic_.init();
    rms_   = 0.f;
    peak_  = 0.f;
    for (int& b : bands_) b = 0;
  }

  void tick(uint32_t /*now_ms*/) override {
    int16_t buf[kFrame];
    const size_t n = mic_.read_samples(buf, kFrame);
    if (n == 0) return;

    // RMS over the whole frame.
    double sum_sq = 0.0;
    int16_t pk = 0;
    for (size_t i = 0; i < n; ++i) {
      const int s = std::abs(buf[i]);
      if (s > pk) pk = s;
      sum_sq += static_cast<double>(buf[i]) * buf[i];
    }
    const float new_rms = static_cast<float>(std::sqrt(sum_sq / n)) / 32767.0f;
    rms_  = 0.7f * rms_  + 0.3f * new_rms;
    peak_ = std::max(peak_ * 0.95f, pk / 32767.0f);

    // 8-band log-spaced spectrum via Hann-windowed radix-2 FFT.
    if (n == kFrame) compute_spectrum_(buf);
  }

  // Bin ranges chosen for 16 kHz sample rate, 256-pt FFT (62.5 Hz/bin).
  // [start, end) over the 0..N/2 magnitude bins.
  static constexpr int kBandBins[kBands + 1] = {1, 2, 4, 8, 16, 32, 64, 96, 128};

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "Mic VU", kWhite, kJapanRed);

    // VU bar (top half)
    const int vu_y = 24;
    const int vu_h = 16;
    const int vu_w = d.width() - 16;
    d.fill_rect({8, vu_y, vu_w, vu_h}, kJapanRedDark);
    const int filled = static_cast<int>(rms_ * vu_w);
    d.fill_rect({8, vu_y, filled, vu_h}, kJapanRed);

    // 8-band envelope (lower half)
    const int band_y = vu_y + vu_h + 6;
    const int band_h = 60;
    const int gap    = 2;
    const int bw     = (d.width() - 16 - (kBands - 1) * gap) / kBands;
    for (int b = 0; b < kBands; ++b) {
      const int v = std::min(100, std::max(0, bands_[b] / 5));  // raw is unclamped log intensity
      const int h = band_h * v / 100;
      const int x = 8 + b * (bw + gap);
      d.fill_rect({x, band_y + (band_h - h), bw, h}, kJapanRed);
    }

    char line[32];
    std::snprintf(line, sizeof(line), "rms %.2f peak %.2f", rms_, peak_);
    d.draw_text(8, d.height() - 14, line, kJapanRedDark, kWhite);
    d.flush();
  }

  float rms()  const { return rms_; }
  float peak() const { return peak_; }
  int   band(int i) const { return bands_[i]; }

private:
  void compute_spectrum_(const int16_t* buf) {
    float re[kFrame];
    float im[kFrame] = {0.f};
    for (size_t i = 0; i < kFrame; ++i) re[i] = static_cast<float>(buf[i]) / 32768.f;
    dsp::hann_window(re, kFrame);
    dsp::fft_radix2(re, im, kFrame);
    // Magnitudes for first half (positive frequencies).
    float mags[kFrame / 2];
    for (size_t k = 0; k < kFrame / 2; ++k) {
      mags[k] = std::sqrt(re[k] * re[k] + im[k] * im[k]);
    }
    // Store unclamped intensities so the peak band is unambiguous even when
    // the visualizer would saturate; the renderer clamps for the bar height.
    for (int b = 0; b < kBands; ++b) {
      float peak = 0.f;
      const int lo = kBandBins[b];
      const int hi = kBandBins[b + 1];
      for (int k = lo; k < hi; ++k) if (mags[k] > peak) peak = mags[k];
      const float scaled = std::log10(1.f + 9.f * peak) * 200.f;
      bands_[b] = static_cast<int>(std::max(0.f, scaled));
    }
  }

  IMic& mic_;
  float rms_  = 0.f;
  float peak_ = 0.f;
  int   bands_[kBands] = {0};
};

}  // namespace yui

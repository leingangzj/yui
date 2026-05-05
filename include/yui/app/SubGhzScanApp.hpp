#pragma once
// SubGhzScanApp — sub-GHz spectrum sweep using the CC1101 on the
// Pingequa Hydra RF Cap 424.
//
// Loop: cycle the chip across N freq bins inside the selected band,
// dwell briefly, sample RSSI, repeat. Body renders one vertical bar
// per bin so peaks read at-a-glance.
//
// Bands the chip covers (CC1101 datasheet): 300-348, 387-464, 779-928
// MHz. We expose the three ISM-friendly slices users actually care
// about for pentest work — 315 (US TPMS), 433 (EU/US remotes), 868
// (EU short-range), 915 (US ISM). User toggles bands with Left/Right.
//
// Cap-absence: if the CC1101 wasn't detected at boot, we render
// Chrome::dialog instead of a scan view. Esc returns to the launcher.

#include "yui/app/App.hpp"
#include "yui/hal/ICc1101.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>

namespace yui {

class SubGhzScanApp : public App {
public:
  struct Band {
    const char* label;
    uint32_t start_hz;
    uint32_t stop_hz;
  };

  // Three default bands; can be expanded later. start/stop are
  // inclusive; we walk in (stop-start)/kBins steps.
  static constexpr Band kBands[] = {
    {"315 MHz",  314'000'000, 316'000'000},  // US TPMS
    {"433 MHz",  433'000'000, 434'000'000},  // EU/US remotes
    {"868 MHz",  867'000'000, 869'000'000},  // EU SRD
    {"915 MHz",  914'000'000, 916'000'000},  // US ISM
  };
  static constexpr int kBandCount = sizeof(kBands) / sizeof(kBands[0]);
  static constexpr int kBins      = 60;   // bars across the body

  // Per-tick budget — we sample one bin per tick to keep the UI
  // responsive. At 30 FPS this finishes a 60-bin sweep in ~2 sec.
  explicit SubGhzScanApp(ICc1101* radio) : radio_(radio) {}
  const char* name() const override { return "Sub-GHz Scan"; }
  Category    category() const override { return Category::Radio; }
  const assets::IconRef* icon() const override { return &assets::icons::kScan(); }

  void on_enter(Hal& /*hal*/) override {
    bin_ = 0;
    peak_dbm_ = -127;
    peak_bin_ = 0;
    if (!radio_ || !radio_->is_present()) {
      mode_ = Mode::CapMissing;
      return;
    }
    mode_ = Mode::Scanning;
    apply_band_();
    for (auto& v : rssi_) v = -127;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (mode_ == Mode::CapMissing) {
      // Any key dismisses; Esc handled by Shell to leave.
      return;
    }
    switch (k.key) {
      case Key::Left:
        if (band_ > 0) { --band_; apply_band_(); reset_sweep_(); }
        break;
      case Key::Right:
        if (band_ + 1 < kBandCount) { ++band_; apply_band_(); reset_sweep_(); }
        break;
      case Key::Enter:
        // Pause/resume by toggling between Scanning and Idle.
        mode_ = (mode_ == Mode::Scanning) ? Mode::Paused : Mode::Scanning;
        break;
      default: break;
    }
  }

  void tick(uint32_t /*now_ms*/) override {
    if (mode_ != Mode::Scanning || !radio_) return;
    // I1 — accumulate kSamplesPerBin readings per bin then advance.
    // The accumulator picks up transient signals that a single 1-tick
    // dwell would miss. With auto-tune on, dwell increases on quiet
    // bins (more samples = more chance to catch a burst) and shrinks
    // on noisy bins (we already know it's hot, move on).
    const Band& b = kBands[band_];
    const uint32_t span = b.stop_hz - b.start_hz;
    const uint32_t hz = b.start_hz + (span * bin_) / kBins;
    if (samples_in_bin_ == 0) radio_->set_frequency_hz(hz);
    const int16_t r = radio_->read_rssi_dbm();
    if (r > sample_peak_)   sample_peak_   = r;
    ++samples_in_bin_;
    const int target = effective_dwell_();
    if (samples_in_bin_ >= target) {
      // Commit the bin's peak as its RSSI value. Average would smear
      // bursts; max preserves them.
      rssi_[bin_] = sample_peak_;
      if (sample_peak_ > peak_dbm_) { peak_dbm_ = sample_peak_; peak_bin_ = bin_; }
      samples_in_bin_ = 0;
      sample_peak_    = -127;
      bin_ = (bin_ + 1) % kBins;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (mode_ == Mode::CapMissing) {
      ui::Chrome::cap_missing_dialog(d, "Sub-GHz Scan", "CC1101", "CS=13");
      return;
    }

    char sub[32];
    std::snprintf(sub, sizeof(sub), "%s%s",
                  kBands[band_].label,
                  mode_ == Mode::Paused ? "  PAUSED" : "");
    ui::Chrome::radio_header(d, "Sub-GHz Scan", sub,
                             radio_ && radio_->is_present() ? 1 : 0, -1);

    // Body: bar chart, baseline = bottom of body.
    const int W = d.width();
    const int top_y = ui::kBodyTopY;
    const int chart_h = (ui::kFooterY - top_y) - 18;  // leave room for stats
    const int base_y = top_y + chart_h;
    const int bar_w = W / kBins;

    // Map RSSI [-110 .. -30] dBm to 0..chart_h pixels.
    auto bar_h = [&](int16_t dbm) {
      if (dbm < -110) dbm = -110;
      if (dbm > -30)  dbm = -30;
      return ((dbm + 110) * chart_h) / 80;
    };

    for (int i = 0; i < kBins; ++i) {
      const int h = bar_h(rssi_[i]);
      const int x = i * bar_w;
      const Color c = (i == peak_bin_) ? ui::kWarn : ui::kAccent;
      d.fill_rect({x, base_y - h, bar_w - 1, h}, c);
    }

    // Stat strip: peak freq + dBm + bin count
    const Band& b = kBands[band_];
    const uint32_t span = b.stop_hz - b.start_hz;
    const uint32_t peak_hz = b.start_hz + (span * peak_bin_) / kBins;
    char line[40];
    std::snprintf(line, sizeof(line), "peak %u.%03u  %d dBm",
                  static_cast<unsigned>(peak_hz / 1'000'000),
                  static_cast<unsigned>((peak_hz / 1000) % 1000),
                  peak_dbm_);
    d.draw_text_styled(ui::kBodyPadX, base_y + 2, line,
                       ui::kHint, ui::kSurface, FontStyle::Caption);

    ui::Chrome::footer(d, "</> band  Enter:pause");
    d.flush();
  }

  // ── Test hooks ──────────────────────────────────────────────────
  enum class Mode { Scanning, Paused, CapMissing };
  Mode mode() const { return mode_; }
  int  band() const { return band_; }
  int16_t peak_dbm() const { return peak_dbm_; }
  int  dwell_samples() const { return dwell_samples_; }
  void set_dwell_samples(int n) {
    dwell_samples_ = (n < 1) ? 1 : (n > 50 ? 50 : n);
  }
  bool auto_tune() const { return auto_tune_; }
  void set_auto_tune(bool on) { auto_tune_ = on; }

private:
  // I1 — effective per-bin sample count. Auto-tune scales by recent
  // RSSI: quiet bin (≤-90 dBm) gets more samples to catch transients;
  // hot bin (≥-50 dBm) needs fewer. Always clamped to [1, 50].
  int effective_dwell_() const {
    if (!auto_tune_) return dwell_samples_;
    const int16_t r = rssi_[bin_];
    int n = dwell_samples_;
    if (r <= -90) n *= 2;
    else if (r >= -50) n = (n + 1) / 2;
    if (n < 1) n = 1;
    if (n > 50) n = 50;
    return n;
  }

  void apply_band_() {
    if (!radio_) return;
    radio_->set_frequency_hz(kBands[band_].start_hz);
  }
  void reset_sweep_() {
    bin_ = 0;
    peak_dbm_ = -127;
    peak_bin_ = 0;
    samples_in_bin_ = 0;
    sample_peak_    = -127;
    for (auto& v : rssi_) v = -127;
  }

  ICc1101* radio_;
  Mode    mode_     = Mode::Scanning;
  int     band_     = 1;          // default 433 MHz
  int     bin_      = 0;
  int16_t peak_dbm_ = -127;
  int     peak_bin_ = 0;
  int16_t rssi_[kBins] = {};
  int     samples_in_bin_ = 0;
  int16_t sample_peak_    = -127;
  int     dwell_samples_  = 5;    // default per-bin samples
  bool    auto_tune_      = false;
};

}  // namespace yui

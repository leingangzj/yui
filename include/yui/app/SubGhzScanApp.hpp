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
    // Step one bin per tick.
    const Band& b = kBands[band_];
    const uint32_t span = b.stop_hz - b.start_hz;
    const uint32_t hz = b.start_hz + (span * bin_) / kBins;
    radio_->set_frequency_hz(hz);
    // RSSI read happens on the chip's last RX sample. RadioLib's CC1101
    // updates its internal RSSI register continuously while in RX, so
    // we don't have to enter/exit modes per bin.
    const int16_t r = radio_->read_rssi_dbm();
    rssi_[bin_] = r;
    if (r > peak_dbm_) { peak_dbm_ = r; peak_bin_ = bin_; }
    bin_ = (bin_ + 1) % kBins;
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

private:
  void apply_band_() {
    if (!radio_) return;
    radio_->set_frequency_hz(kBands[band_].start_hz);
  }
  void reset_sweep_() {
    bin_ = 0;
    peak_dbm_ = -127;
    peak_bin_ = 0;
    for (auto& v : rssi_) v = -127;
  }

  ICc1101* radio_;
  Mode    mode_     = Mode::Scanning;
  int     band_     = 1;          // default 433 MHz
  int     bin_      = 0;
  int16_t peak_dbm_ = -127;
  int     peak_bin_ = 0;
  int16_t rssi_[kBins] = {};
};

}  // namespace yui

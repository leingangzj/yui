#pragma once
// Nrf24ScanApp — 2.4 GHz channel-activity scanner.
//
// Loop: hop the chip channel-by-channel (0..125), peek at carrier-
// detect, accumulate hits-per-channel into a heatmap. Body shows one
// vertical bar per channel; height = recent activity over a decay
// window so peaks fade if the source goes quiet.

#include "yui/app/App.hpp"
#include "yui/hal/INrf24.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>

namespace yui {

class Nrf24ScanApp : public App {
public:
  static constexpr int kChannels = 126;   // 2400..2525 MHz, 1 MHz/ch

  explicit Nrf24ScanApp(INrf24* radio) : radio_(radio) {}
  const char* name() const override { return "NRF24 Scan"; }
  Category    category() const override { return Category::Bluetooth; }

  void on_enter(Hal& /*hal*/) override {
    if (!radio_ || !radio_->is_present()) {
      mode_ = Mode::CapMissing;
      return;
    }
    mode_ = Mode::Scanning;
    ch_ = 0;
    for (auto& v : hits_) v = 0;
    radio_->set_data_rate(NrfDataRate::Rate1Mbps);
  }

  void on_key(KeyEvent k) override {
    if (!k.down || mode_ == Mode::CapMissing) return;
    if (k.key == Key::Enter) {
      mode_ = (mode_ == Mode::Scanning) ? Mode::Paused : Mode::Scanning;
    }
  }

  void tick(uint32_t /*now_ms*/) override {
    if (mode_ != Mode::Scanning || !radio_) return;
    // Step a few channels per tick — RadioLib's nRF24 channel switch
    // is fast (microseconds), so we can cover ~10 channels per frame
    // and still leave plenty of frame budget for rendering.
    for (int i = 0; i < 10; ++i) {
      radio_->set_channel(static_cast<uint8_t>(ch_));
      radio_->start_listening();
      // Brief dwell isn't strictly needed — carrier_detected() reads
      // the chip's RPD bit which latches over the listen window.
      if (radio_->carrier_detected()) {
        if (hits_[ch_] < 255) ++hits_[ch_];
      } else if (hits_[ch_] > 0) {
        // Slow decay so old peaks fade rather than disappearing
        // every sweep.
        --hits_[ch_];
      }
      ch_ = (ch_ + 1) % kChannels;
    }
    radio_->stop_listening();
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (mode_ == Mode::CapMissing) {
      ui::Chrome::cap_missing_dialog(d, "NRF24 Scan", "nRF24", "CS=6");
      return;
    }

    const char* sub = (mode_ == Mode::Paused) ? "PAUSED" : nullptr;
    ui::Chrome::radio_header(d, "NRF24 Scan", sub, -1,
                             radio_ && radio_->is_present() ? 1 : 0);

    const int W = d.width();
    const int top_y = ui::kBodyTopY;
    const int chart_h = (ui::kFooterY - top_y) - 18;
    const int base_y = top_y + chart_h;

    // 126 channels in 240 px → ~1.9 px/bar; quantize to ints.
    int peak_ch = 0;
    uint8_t peak_v = 0;
    for (int i = 0; i < kChannels; ++i) {
      if (hits_[i] > peak_v) { peak_v = hits_[i]; peak_ch = i; }
    }
    for (int i = 0; i < kChannels; ++i) {
      const int v = hits_[i];
      if (v == 0) continue;
      const int h = (v * chart_h) / 32;     // saturate at hits=32 → full
      const int hh = h > chart_h ? chart_h : h;
      const int x = (i * W) / kChannels;
      const int next_x = ((i + 1) * W) / kChannels;
      const int bw = next_x - x - 1;
      const Color c = (i == peak_ch) ? ui::kWarn : ui::kAccent;
      d.fill_rect({x, base_y - hh, bw > 0 ? bw : 1, hh}, c);
    }

    char line[40];
    std::snprintf(line, sizeof(line),
                  "peak ch %d  %d MHz  hits=%u",
                  peak_ch, 2400 + peak_ch, peak_v);
    d.draw_text_styled(ui::kBodyPadX, base_y + 2, line,
                       ui::kHint, ui::kSurface, FontStyle::Caption);

    ui::Chrome::footer(d, "Enter:pause  Esc:back");
    d.flush();
  }

  // Test hooks
  enum class Mode { Scanning, Paused, CapMissing };
  Mode mode() const { return mode_; }
  int  current_channel() const { return ch_; }

private:
  INrf24* radio_;
  Mode    mode_ = Mode::Scanning;
  int     ch_   = 0;
  uint8_t hits_[kChannels] = {};
};

}  // namespace yui

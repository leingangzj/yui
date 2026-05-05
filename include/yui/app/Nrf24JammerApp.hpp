#pragma once
// Nrf24JammerApp — 2.4 GHz denial modes via the nRF24L01+.
//
// Phase 4 modes: ChannelFlood (single-channel carrier), HopJam (cycle
// across all 126 channels), TargetMac (jam scoped to a sniffed MAC's
// hop sequence — placeholder until MousejackApp lands).

#include "yui/app/App.hpp"
#include "yui/hal/INrf24.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>

namespace yui {

class Nrf24JammerApp : public App {
public:
  static constexpr const char* kModes[] = {
    "Channel flood", "Hop jam", "Target MAC"
  };
  static constexpr int kModeCount = sizeof(kModes) / sizeof(kModes[0]);

  explicit Nrf24JammerApp(INrf24* radio) : radio_(radio) {}
  const char* name() const override { return "NRF24 Jam"; }
  Category    category() const override { return Category::Bluetooth; }

  void on_enter(Hal& /*hal*/) override {
    cap_missing_ = (!radio_ || !radio_->is_present());
    active_ = false;
    if (!cap_missing_) {
      radio_->set_channel(static_cast<uint8_t>(channel_));
      radio_->set_carrier(false);
    }
  }

  void on_exit() override {
    if (radio_) radio_->set_carrier(false);
    active_ = false;
  }

  void on_key(KeyEvent k) override {
    if (!k.down || cap_missing_) return;
    switch (k.key) {
      case Key::Up:    if (mode_ > 0) --mode_; break;
      case Key::Down:  if (mode_ + 1 < kModeCount) ++mode_; break;
      case Key::Left:  if (channel_ > 0) { --channel_; tune_(); } break;
      case Key::Right: if (channel_ < 125) { ++channel_; tune_(); } break;
      case Key::Enter:
        active_ = !active_;
        // Channel flood is hot in Phase 4; hop + target arrive in 4.5.
        if (mode_ == 0 && radio_) radio_->set_carrier(active_);
        break;
      default: break;
    }
  }

  void tick(uint32_t /*now_ms*/) override {
    if (!active_ || !radio_) return;
    if (mode_ == 1) {
      // Hop jam: cycle channels every tick. The carrier stays asserted
      // through the tune so we get a saw-tooth across the band.
      channel_ = (channel_ + 1) % 126;
      radio_->set_channel(static_cast<uint8_t>(channel_));
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (cap_missing_) {
      ui::Chrome::cap_missing_dialog(d, "NRF24 Jam", "nRF24", "CS=6");
      return;
    }
    ui::Chrome::radio_header(d, "NRF24 Jam",
                             active_ ? "ACTIVE" : nullptr, -1,
                             radio_ && radio_->is_present() ? 1 : 0);
    for (int i = 0; i < kModeCount; ++i) {
      ui::Chrome::list_row(d, i, kModes[i], i == mode_);
    }

    char line[24];
    std::snprintf(line, sizeof(line), "ch %d  (%d MHz)",
                  channel_, 2400 + channel_);
    d.draw_text_styled(ui::kBodyPadX, ui::kFooterY - 14, line,
                       ui::kHint, ui::kSurface, FontStyle::Caption);

    ui::Chrome::footer(d, active_ ? "Enter:STOP  </>:ch"
                                  : "Enter:start  </>:ch");
    d.flush();
  }

  bool cap_missing() const { return cap_missing_; }
  bool active() const { return active_; }
  int  mode()   const { return mode_; }

private:
  void tune_() { if (radio_) radio_->set_channel(static_cast<uint8_t>(channel_)); }

  INrf24* radio_;
  bool    cap_missing_ = false;
  bool    active_ = false;
  int     mode_   = 0;
  int     channel_ = 76;
};

}  // namespace yui

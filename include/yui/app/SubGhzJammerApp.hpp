#pragma once
// SubGhzJammerApp — denial-of-service modes for the CC1101.
//
// Modes (per Phase 4 plan): CW (continuous carrier), Noise (random
// bytes streamed), Sweep (frequency-hop across the band), Protocol
// (modulation-matched), Reactive (TX on preamble detect).
//
// Phase 4 scaffold lands CW (live; one call to set_carrier(true))
// plus the mode-picker UI; richer modes arrive in 4.5.

#include "yui/app/App.hpp"
#include "yui/hal/ICc1101.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>

namespace yui {

class SubGhzJammerApp : public App {
public:
  static constexpr const char* kModes[] = {
    "CW (carrier)", "Noise", "Sweep", "Protocol", "Reactive"
  };
  static constexpr int kModeCount = sizeof(kModes) / sizeof(kModes[0]);

  explicit SubGhzJammerApp(ICc1101* radio) : radio_(radio) {}
  const char* name() const override { return "Sub-GHz Jam"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& /*hal*/) override {
    cap_missing_ = (!radio_ || !radio_->is_present());
    active_      = false;
    mode_        = 0;
    if (!cap_missing_) {
      radio_->set_frequency_hz(freq_hz_);
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
      case Key::Left:  if (freq_hz_ > 300'000'000) { freq_hz_ -= 100'000; tune_(); } break;
      case Key::Right: if (freq_hz_ < 928'000'000) { freq_hz_ += 100'000; tune_(); } break;
      case Key::Enter:
        active_ = !active_;
        // Phase 4 hot path: CW mode lights up the chip immediately;
        // other modes show the toggle but don't stream until 4.5.
        if (mode_ == 0 && radio_) radio_->set_carrier(active_);
        break;
      default: break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (cap_missing_) {
      ui::Chrome::header(d, "Sub-GHz Jam", "no cap");
      ui::Chrome::dialog(d, "Hydra not found",
                         "CC1101 did not respond. Re-seat the cap.",
                         "OK", nullptr, true);
      d.flush();
      return;
    }
    ui::Chrome::header(d, "Sub-GHz Jam", active_ ? "ACTIVE" : nullptr);

    // Mode picker (4 visible at a time; arrows scroll selection).
    for (int i = 0; i < kModeCount; ++i) {
      ui::Chrome::list_row(d, i, kModes[i], i == mode_);
    }

    char line[32];
    std::snprintf(line, sizeof(line), "%u.%03u MHz",
                  static_cast<unsigned>(freq_hz_ / 1'000'000),
                  static_cast<unsigned>((freq_hz_ / 1000) % 1000));
    d.draw_text_styled(ui::kBodyPadX,
                       ui::kFooterY - 14,
                       line, ui::kHint, ui::kSurface, FontStyle::Caption);

    ui::Chrome::footer(d, active_ ? "Enter:STOP  </>:tune"
                                  : "Enter:start  </>:tune");
    d.flush();
  }

  bool cap_missing() const { return cap_missing_; }
  bool active() const { return active_; }
  int  mode() const { return mode_; }

private:
  void tune_() { if (radio_) radio_->set_frequency_hz(freq_hz_); }

  ICc1101* radio_;
  bool     cap_missing_ = false;
  bool     active_      = false;
  int      mode_        = 0;
  uint32_t freq_hz_     = 433'920'000;
};

}  // namespace yui

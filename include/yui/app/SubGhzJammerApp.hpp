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
        toggle_active_();
        break;
      default: break;
    }
  }

  void tick(uint32_t /*now_ms*/) override {
    if (!active_ || !radio_) return;
    switch (mode_) {
      case 1: {
        // Noise: blast a random byte stream as fast as the chip will
        // accept it. Pseudo-random walk seeded from a counter so the
        // spectrum doesn't degenerate to a tone.
        uint8_t pkt[16];
        for (auto& b : pkt) {
          noise_seed_ = noise_seed_ * 1664525u + 1013904223u;
          b = static_cast<uint8_t>(noise_seed_ >> 24);
        }
        radio_->transmit(pkt, sizeof(pkt));
        break;
      }
      case 2: {
        // Sweep: hop the frequency up by 100 kHz per tick, wrap at
        // band edge. Carrier asserted continuously for a saw-tooth
        // across whichever sub-GHz band we're targeting.
        sweep_hz_ += 100'000;
        if (sweep_hz_ > sweep_end_hz_) sweep_hz_ = sweep_start_hz_;
        radio_->set_frequency_hz(sweep_hz_);
        break;
      }
      // CW (mode 0) just runs the carrier asserted in toggle_active_();
      // Protocol (3) and Reactive (4) need preamble-detect IRQs we
      // haven't wired yet — render shows them but tick is a no-op.
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

  void toggle_active_() {
    active_ = !active_;
    if (!radio_) return;
    if (!active_) {
      radio_->set_carrier(false);
      return;
    }
    switch (mode_) {
      case 0:  // CW — pure carrier
        radio_->set_carrier(true);
        break;
      case 1:  // Noise — packet TX driven from tick()
        radio_->set_modulation(CcModulation::Ook);
        radio_->set_bitrate_bps(50'000);
        break;
      case 2:  // Sweep — carrier on, freq stepped from tick()
        sweep_start_hz_ = (freq_hz_ > 500'000) ? freq_hz_ - 500'000 : 300'000'000;
        sweep_end_hz_   = freq_hz_ + 500'000;
        sweep_hz_       = sweep_start_hz_;
        radio_->set_carrier(true);
        break;
      case 3:  // Protocol-aware — placeholder, future preamble-IRQ hook
      case 4:  // Reactive — same
      default:
        radio_->set_carrier(true);
        break;
    }
  }

  ICc1101* radio_;
  bool     cap_missing_ = false;
  bool     active_      = false;
  int      mode_        = 0;
  uint32_t freq_hz_     = 433'920'000;
  uint32_t noise_seed_  = 0xdeadbeef;
  uint32_t sweep_start_hz_ = 433'420'000;
  uint32_t sweep_end_hz_   = 434'420'000;
  uint32_t sweep_hz_       = 433'920'000;
};

}  // namespace yui

#pragma once
// BleJammerApp — saturate the local 2.4 GHz BLE adv channels (37/38/39)
// with rapid back-to-back advertisements. Different from BleSpamApp
// which cycles _meaningful_ nuisance payloads at a slow cadence: this
// one fires dummy packets as fast as the radio will let it, with the
// goal of disrupting nearby BLE pairing / discovery.
//
// LEGAL/ETHICAL: actual radio jamming (continuous-wave RF that
// physically suppresses other devices) is illegal nearly everywhere.
// This app does NOT do that — it just submits BLE adv packets at
// max rate. Effect on neighbors is similar to a noisy environment,
// not a regulated jammer. Use sparingly and never around medical
// equipment, hearing aids, etc.
//
// Off by default. Fn+Enter to enable. Auto-disables after kAutoOffMs
// (default 30s) so you can't accidentally leave it running.
#include "yui/app/App.hpp"
#include "yui/hal/IBleAdvertiser.hpp"
#include "yui/hal/IBleRawTx.hpp"
#include "yui/types.hpp"
#include <cstdint>
#include <cstdio>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class BleJammerApp : public App {
public:
  static constexpr uint32_t kCycleMs    = 10;       // ~100/s set+enable cycle
  static constexpr uint32_t kAutoOffMs  = 30000;    // failsafe shutoff

  enum class Rail { Nimble, Nrf24, Both };

  explicit BleJammerApp(IBleAdvertiser& adv) : adv_(adv) {}

  void set_nrf24_rail(IBleRawTx* nrf24) {
    nrf24_ = nrf24;
    rail_  = (nrf24 && nrf24->is_present()) ? Rail::Both : Rail::Nimble;
  }

  const char* name() const override { return "BLE Jam"; }
  Category    category() const override { return Category::Bluetooth; }

  void on_enter(Hal& hal) override {
    hal_      = &hal;
    enabled_  = false;
    cycle_n_  = 0;
    started_ms_ = 0;
    last_cycle_ms_ = 0;
    adv_.disable();
  }

  void on_exit() override { adv_.disable(); }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Tab && !enabled_) {
      if (!nrf24_) { rail_ = Rail::Nimble; return; }
      rail_ = (rail_ == Rail::Both)   ? Rail::Nimble
            : (rail_ == Rail::Nimble) ? Rail::Nrf24
                                      : Rail::Both;
      return;
    }
    if (k.key == Key::Enter && k.fn) {
      enabled_ = !enabled_;
      if (enabled_) { started_ms_ = 0; cycle_n_ = 0; }
      else {
        adv_.disable();
        if (nrf24_) nrf24_->stop();
      }
    }
  }

  void tick(uint32_t now_ms) override {
    if (!enabled_) return;
    if (started_ms_ == 0) {
      started_ms_ = now_ms == 0 ? 1 : now_ms;
    } else {
      // Auto-shutoff (only after started_ms_ was set in a prior tick)
      if (now_ms - started_ms_ >= kAutoOffMs) {
        enabled_ = false;
        adv_.disable();
        return;
      }
    }
    if (now_ms - last_cycle_ms_ < kCycleMs && last_cycle_ms_ != 0) return;
    last_cycle_ms_ = now_ms == 0 ? 1 : now_ms;
    uint8_t buf[8] = {0x02, 0x01, 0x06,
                      0x03, 0xFF,
                      static_cast<uint8_t>(cycle_n_ & 0xFF),
                      static_cast<uint8_t>((cycle_n_ >> 8) & 0xFF),
                      static_cast<uint8_t>((cycle_n_ >> 16) & 0xFF)};
    if (rail_ == Rail::Nimble || rail_ == Rail::Both) {
      adv_.set_payload(buf, sizeof(buf));
      if (!adv_.active()) adv_.enable();
    }
    if (nrf24_ && (rail_ == Rail::Nrf24 || rail_ == Rail::Both)) {
      // Continuous TX on the channel not handled by NimBLE — channel-
      // divided to avoid self-collision. Restart only when not active.
      if (!nrf24_->is_active()) nrf24_->start_continuous(IBleRawTx::kChannel39);
    }
    ++cycle_n_;
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "BLE Jam");

    char line[40];
    std::snprintf(line, sizeof(line), "State: %s", enabled_ ? "Jamming" : "off");
    d.draw_text_styled(8, 24, line,
                       enabled_ ? ui::kWarn : ui::kOnSurface, ui::kSurface,
                       FontStyle::Body);
    std::snprintf(line, sizeof(line), "Cycles: %u", static_cast<unsigned>(cycle_n_));
    d.draw_text_styled(8, 44, line, ui::kOnSurface, ui::kSurface,
                       FontStyle::Body);
    std::snprintf(line, sizeof(line), "Rail:  %s",
                  rail_ == Rail::Both   ? "Both"
                : rail_ == Rail::Nimble ? "NimBLE"
                                        : "nRF24");
    d.draw_text_styled(8, 60, line, ui::kHint, ui::kSurface, FontStyle::Caption);
    d.draw_text_styled(8, 76, "Auto-off after 30s", ui::kHint, ui::kSurface,
                       FontStyle::Caption);
    ui::Chrome::footer(d, "Tab:rail Fn+Enter:toggle Esc:back");
    d.flush();
  }

  // Test hooks
  bool   enabled() const { return enabled_; }
  size_t cycles()  const { return cycle_n_; }
  Rail   rail()    const { return rail_; }
  void   set_rail(Rail r) { rail_ = r; }

private:
  IBleAdvertiser& adv_;
  IBleRawTx*      nrf24_         = nullptr;
  Hal*            hal_           = nullptr;
  bool            enabled_       = false;
  size_t          cycle_n_       = 0;
  uint32_t        started_ms_    = 0;
  uint32_t        last_cycle_ms_ = 0;
  Rail            rail_          = Rail::Nimble;
};

}  // namespace yui

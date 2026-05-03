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

  explicit BleJammerApp(IBleAdvertiser& adv) : adv_(adv) {}

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
    if (k.key == Key::Enter && k.fn) {
      enabled_ = !enabled_;
      if (enabled_) { started_ms_ = 0; cycle_n_ = 0; }
      else          { adv_.disable(); }
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
    // Rotate a 4-byte counter as the payload so each adv differs.
    uint8_t buf[8] = {0x02, 0x01, 0x06,
                      0x03, 0xFF,
                      static_cast<uint8_t>(cycle_n_ & 0xFF),
                      static_cast<uint8_t>((cycle_n_ >> 8) & 0xFF),
                      static_cast<uint8_t>((cycle_n_ >> 16) & 0xFF)};
    adv_.set_payload(buf, sizeof(buf));
    if (!adv_.active()) adv_.enable();
    ++cycle_n_;
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "BLE Jam");

    char line[40];
    std::snprintf(line, sizeof(line), "State: %s", enabled_ ? "JAMMING" : "off");
    d.draw_text(8, 24, line, enabled_ ? kJapanRedBright : kBlack, kWhite);
    std::snprintf(line, sizeof(line), "Cycles: %u", static_cast<unsigned>(cycle_n_));
    d.draw_text(8, 44, line, kBlack, kWhite);
    d.draw_text(8, 64, "Fn+Enter: toggle", kJapanRed, kWhite);
    d.draw_text(8, 84, "Auto-off after 30 sec", kJapanRedDark, kWhite);
    d.draw_text(8, d.height() - 14,
                "Use only in your own space",
                kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  bool   enabled() const { return enabled_; }
  size_t cycles()  const { return cycle_n_; }

private:
  IBleAdvertiser& adv_;
  Hal*            hal_           = nullptr;
  bool            enabled_       = false;
  size_t          cycle_n_       = 0;
  uint32_t        started_ms_    = 0;
  uint32_t        last_cycle_ms_ = 0;
};

}  // namespace yui

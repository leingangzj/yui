#pragma once
// BleSpamApp — cycles through "nuisance" BLE advertisement payloads:
// Apple proximity / Samsung Easy Setup / Google Fast Pair-style
// announcements. Off by default; requires Fn+Enter to enable. Useful
// for demos in a controlled environment to demonstrate why BLE
// adv-spam is annoying for nearby devices.
//
// LEGAL/ETHICAL: this is technically benign (passive advertising) but
// disrupts nearby BT pairing UX and can be considered nuisance use of
// the airwaves. Use sparingly, never in public, never around someone
// else's pairing-in-progress.
#include "yui/app/App.hpp"
#include "yui/hal/IBleAdvertiser.hpp"
#include "yui/types.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class BleSpamApp : public App {
public:
  static constexpr uint32_t kCycleMs = 200;

  // Three small, well-known nuisance payloads. Each is a flat byte
  // array fed verbatim to the BLE adv data slot. The payloads are
  // deliberately simple (length-tagged adv data records) and are
  // public knowledge — included here for awareness/testing.
  struct PayloadInfo { const char* label; const uint8_t* bytes; size_t len; };

  static const PayloadInfo* payloads(size_t& count) {
    // Public BLE adv data layout: <len><type><body>... (length includes type)
    // Apple iBeacon-like marker (vendor-specific, Apple manufacturer ID 0x004C)
    static const uint8_t kApple[] = {
      0x02, 0x01, 0x06,                        // flags: LE general
      0x1A, 0xFF, 0x4C, 0x00,                  // mfg-data, Apple
      0x02, 0x15,                              // proximity (iBeacon header)
      0xE2, 0xC5, 0x6D, 0xB5, 0xDF, 0xFB, 0x48, 0xD2,
      0xB0, 0x60, 0xD0, 0xF5, 0xA7, 0x10, 0x96, 0xE0,
      0x00, 0x00, 0x00, 0x00, 0xC5,
    };
    // Samsung mfg-id 0x0075
    static const uint8_t kSamsung[] = {
      0x02, 0x01, 0x06,
      0x06, 0xFF, 0x75, 0x00, 0x42, 0x09, 0x81,
    };
    // Google Fast Pair model-id placeholder, mfg-id 0x00E0
    static const uint8_t kGoogle[] = {
      0x02, 0x01, 0x06,
      0x05, 0xFF, 0xE0, 0x00, 0x00, 0x00,
    };
    static const PayloadInfo k[] = {
      {"Apple",   kApple,   sizeof(kApple)},
      {"Samsung", kSamsung, sizeof(kSamsung)},
      {"Google",  kGoogle,  sizeof(kGoogle)},
    };
    count = sizeof(k) / sizeof(k[0]);
    return k;
  }

  explicit BleSpamApp(IBleAdvertiser& adv) : adv_(adv) {}

  const char* name() const override { return "BLE Spam"; }
  Category    category() const override { return Category::Bluetooth; }

  void on_enter(Hal& hal) override {
    hal_           = &hal;
    enabled_       = false;
    last_cycle_ms_ = 0;
    cycle_idx_     = 0;
    adv_.disable();
  }

  void on_exit() override { adv_.disable(); }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Enter && k.fn) {
      enabled_ = !enabled_;
      if (!enabled_) adv_.disable();
    }
  }

  void tick(uint32_t now_ms) override {
    if (!enabled_) return;
    if (now_ms - last_cycle_ms_ < kCycleMs && last_cycle_ms_ != 0) return;
    last_cycle_ms_ = now_ms == 0 ? 1 : now_ms;
    size_t n = 0;
    const PayloadInfo* list = payloads(n);
    cycle_idx_ = (cycle_idx_ + 1) % n;
    const PayloadInfo& p = list[cycle_idx_];
    adv_.set_payload(p.bytes, p.len);
    if (!adv_.active()) adv_.enable();
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "BLE Spam");

    char line[40];
    std::snprintf(line, sizeof(line), "State: %s",
                  enabled_ ? "ON" : "off");
    d.draw_text(8, 24, line, enabled_ ? kJapanRedBright : ui::kOnSurface, ui::kSurface);

    size_t n = 0;
    const PayloadInfo* list = payloads(n);
    std::snprintf(line, sizeof(line), "Now:   %s",
                  enabled_ ? list[cycle_idx_].label : "—");
    d.draw_text(8, 44, line, ui::kOnSurface, ui::kSurface);

    d.draw_text(8, 64, "Fn+Enter: toggle", ui::kAccent,     ui::kSurface);
    ui::Chrome::footer(d, "Use only in your own space");
    d.flush();
  }

  // Test hooks
  bool   enabled()   const { return enabled_; }
  size_t cycle_idx() const { return cycle_idx_; }

private:
  IBleAdvertiser& adv_;
  Hal*            hal_           = nullptr;
  bool            enabled_       = false;
  uint32_t        last_cycle_ms_ = 0;
  size_t          cycle_idx_     = 0;
};

}  // namespace yui

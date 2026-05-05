#pragma once
// BleSpamApp — cycles three nuisance BLE adv payloads (Apple proximity,
// Samsung Easy Setup, Google Fast Pair). Off by default; Fn+Enter
// enables. Useful for demoing why BLE adv-spam is annoying for nearby
// devices.
//
// Technically benign (passive advertising) but disrupts nearby BT
// pairing UX. Use only in your own space, never around someone else's
// pairing-in-progress.
//
// Phase 5.2 added an optional second-rail nRF24 path via set_nrf24_rail.
// Tab cycles Rail (Both / NimBLE / Nrf24) when not running. Default is
// Nimble-only when the nRF24 rail isn't wired.
#include "yui/app/App.hpp"
#include "yui/hal/IBleAdvertiser.hpp"
#include "yui/hal/IBleRawTx.hpp"
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

  enum class Rail { Nimble, Nrf24, Both };

  explicit BleSpamApp(IBleAdvertiser& adv) : adv_(adv) {}

  // Optional second-rail wiring (Phase 5.2). Pass an IBleRawTx for the
  // nRF24-on-Hydra path; rail() then defaults to Both when both rails
  // are present, Nimble otherwise.
  void set_nrf24_rail(IBleRawTx* nrf24) {
    nrf24_ = nrf24;
    rail_  = (nrf24 && nrf24->is_present()) ? Rail::Both : Rail::Nimble;
  }

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
    if (k.key == Key::Tab && !enabled_) {
      // Cycle rail when not running. Both → Nimble → Nrf24 → Both ...
      // Skips Nrf24 / Both if no nrf24 rail is wired.
      if (!nrf24_) { rail_ = Rail::Nimble; return; }
      rail_ = (rail_ == Rail::Both)   ? Rail::Nimble
            : (rail_ == Rail::Nimble) ? Rail::Nrf24
                                      : Rail::Both;
      return;
    }
    if (k.key == Key::Enter && k.fn) {
      enabled_ = !enabled_;
      if (!enabled_) {
        adv_.disable();
        if (nrf24_) nrf24_->stop();
      }
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
    // Channel-divided default: NimBLE drives ch 37, nRF24 drives 38+39.
    if (rail_ == Rail::Nimble || rail_ == Rail::Both) {
      adv_.set_payload(p.bytes, p.len);
      if (!adv_.active()) adv_.enable();
    }
    if (nrf24_ && (rail_ == Rail::Nrf24 || rail_ == Rail::Both)) {
      // Alternate between channel 38 and 39 each cycle.
      const int ch = (cycle_idx_ & 1) ? IBleRawTx::kChannel38
                                      : IBleRawTx::kChannel39;
      nrf24_->set_channel(ch);
      nrf24_->tx_advert(p.bytes, p.len);
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "BLE Spam");

    char line[40];
    std::snprintf(line, sizeof(line), "State: %s",
                  enabled_ ? "ON" : "off");
    d.draw_text_styled(8, 24, line,
                       enabled_ ? ui::kWarn : ui::kOnSurface, ui::kSurface,
                       FontStyle::Body);

    size_t n = 0;
    const PayloadInfo* list = payloads(n);
    std::snprintf(line, sizeof(line), "Now:   %s",
                  enabled_ ? list[cycle_idx_].label : "—");
    d.draw_text_styled(8, 44, line, ui::kOnSurface, ui::kSurface,
                       FontStyle::Body);

    std::snprintf(line, sizeof(line), "Rail:  %s",
                  rail_ == Rail::Both   ? "Both"
                : rail_ == Rail::Nimble ? "NimBLE"
                                        : "nRF24");
    d.draw_text_styled(8, 60, line, ui::kHint, ui::kSurface, FontStyle::Caption);
    ui::Chrome::footer(d, "Tab:rail Fn+Enter:toggle Esc:back");
    d.flush();
  }

  // Test hooks
  bool   enabled()   const { return enabled_; }
  size_t cycle_idx() const { return cycle_idx_; }
  Rail   rail()      const { return rail_; }
  void   set_rail(Rail r) { rail_ = r; }

private:
  IBleAdvertiser& adv_;
  IBleRawTx*      nrf24_         = nullptr;
  Hal*            hal_           = nullptr;
  bool            enabled_       = false;
  uint32_t        last_cycle_ms_ = 0;
  size_t          cycle_idx_     = 0;
  Rail            rail_          = Rail::Nimble;
};

}  // namespace yui

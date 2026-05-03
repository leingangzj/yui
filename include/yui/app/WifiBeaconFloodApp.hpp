#pragma once
// WifiBeaconFloodApp — broadcast a rotating list of SSIDs as fake APs.
// Uses IWifiMonitor::tx_raw with beacon frames built by Dot11. Each
// SSID gets its own deterministic BSSID derived from the SSID string,
// so nearby clients see a stable list rather than churning.
//
// LEGAL: this transmits 802.11 management frames, which is a
// regulated activity in some jurisdictions. Use only on RF spectrum
// you control / in a test environment / on your own property.
#include "yui/app/App.hpp"
#include "yui/hal/IWifiMonitor.hpp"
#include "yui/proto/Dot11.hpp"
#include "yui/types.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class WifiBeaconFloodApp : public App {
public:
  static constexpr size_t kMaxSsids = 16;
  static constexpr uint32_t kPerBeaconMs = 50;

  // Default SSID set — the kind of names you'd see on a Bruce demo.
  // Caller can override via add_ssid() before on_enter.
  static const char* const* default_ssids(size_t& count) {
    static const char* k[] = {
      "Free WiFi",
      "Starbucks",
      "xfinitywifi",
      "GuestNet",
      "iPhone",
      "AndroidAP",
      "Press F to pay respects",
      "FBI Surveillance Van",
    };
    count = sizeof(k) / sizeof(k[0]);
    return k;
  }

  explicit WifiBeaconFloodApp(IWifiMonitor& mon) : mon_(mon) {}

  const char* name() const override { return "Beacon Flood"; }
  Category    category() const override { return Category::WiFi; }

  void on_enter(Hal& hal) override {
    hal_         = &hal;
    enabled_     = false;
    idx_         = 0;
    tx_total_    = 0;
    last_tx_ms_  = 0;
    if (ssid_count_ == 0) {
      size_t n = 0;
      const char* const* defs = default_ssids(n);
      for (size_t i = 0; i < n && ssid_count_ < kMaxSsids; ++i) {
        std::strncpy(ssids_[ssid_count_], defs[i], 32);
        ssids_[ssid_count_][32] = '\0';
        ++ssid_count_;
      }
    }
    WifiFilter f{}; f.mgmt = false; f.ctrl = false; f.data = false;  // TX-only
    mon_.start(f, channel_);
  }

  void on_exit() override { mon_.stop(); }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Tab || (k.key == Key::Enter && k.fn)) {
      enabled_ = !enabled_;
    }
    if (k.key == Key::Up   && channel_ < 13) { ++channel_; mon_.set_channel(channel_); }
    if (k.key == Key::Down && channel_ > 1)  { --channel_; mon_.set_channel(channel_); }
  }

  void tick(uint32_t now_ms) override {
    if (!enabled_ || ssid_count_ == 0) return;
    if (now_ms - last_tx_ms_ < kPerBeaconMs && last_tx_ms_ != 0) return;
    last_tx_ms_ = now_ms == 0 ? 1 : now_ms;
    const char* ssid = ssids_[idx_];
    uint8_t bssid[6];
    derive_bssid_(ssid, bssid);
    uint8_t frame[256];
    const size_t n = dot11::build_beacon(frame, sizeof(frame),
                                         ssid, std::strlen(ssid),
                                         bssid, channel_);
    if (n > 0 && mon_.tx_raw(frame, n)) ++tx_total_;
    idx_ = (idx_ + 1) % ssid_count_;
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "Beacon Flood");

    char line[40];
    std::snprintf(line, sizeof(line), "State: %s",
                  enabled_ ? "FLOODING" : "idle");
    d.draw_text(8, 22, line, enabled_ ? ui::kWarn : ui::kOnSurface, ui::kSurface);
    std::snprintf(line, sizeof(line), "Channel: %u   SSIDs: %u",
                  static_cast<unsigned>(channel_),
                  static_cast<unsigned>(ssid_count_));
    d.draw_text(8, 38, line, ui::kOnSurface, ui::kSurface);
    std::snprintf(line, sizeof(line), "Sent: %u", static_cast<unsigned>(tx_total_));
    d.draw_text(8, 54, line, ui::kOnSurface, ui::kSurface);
    if (ssid_count_ > 0) {
      std::snprintf(line, sizeof(line), "Now: %.20s", ssids_[idx_]);
      d.draw_text(8, 70, line, ui::kAccent, ui::kSurface);
    }
    ui::Chrome::footer(d, "Tab:toggle Up/Dn:ch");
    d.flush();
  }

  // Configuration before on_enter
  void add_ssid(const char* ssid) {
    if (!ssid || ssid_count_ >= kMaxSsids) return;
    std::strncpy(ssids_[ssid_count_], ssid, 32);
    ssids_[ssid_count_][32] = '\0';
    ++ssid_count_;
  }
  void clear_ssids() { ssid_count_ = 0; }

  // Test hooks
  bool   enabled()    const { return enabled_; }
  size_t tx_total()   const { return tx_total_; }
  size_t ssid_count() const { return ssid_count_; }
  uint8_t channel()   const { return channel_; }

private:
  static void derive_bssid_(const char* ssid, uint8_t out[6]) {
    // FNV-like hash so each SSID has a stable, unique-ish MAC.
    uint64_t h = 0xCBF29CE484222325ULL;
    for (const char* p = ssid; *p; ++p) {
      h ^= static_cast<uint8_t>(*p);
      h *= 0x100000001B3ULL;
    }
    out[0] = 0x02;   // locally administered
    out[1] = static_cast<uint8_t>((h >> 8)  & 0xFF);
    out[2] = static_cast<uint8_t>((h >> 16) & 0xFF);
    out[3] = static_cast<uint8_t>((h >> 24) & 0xFF);
    out[4] = static_cast<uint8_t>((h >> 32) & 0xFF);
    out[5] = static_cast<uint8_t>((h >> 40) & 0xFF);
  }

  IWifiMonitor& mon_;
  Hal*          hal_         = nullptr;
  bool          enabled_     = false;
  uint8_t       channel_     = 6;
  size_t        idx_         = 0;
  size_t        tx_total_    = 0;
  uint32_t      last_tx_ms_  = 0;
  size_t        ssid_count_  = 0;
  char          ssids_[kMaxSsids][33] = {{0}};
};

}  // namespace yui

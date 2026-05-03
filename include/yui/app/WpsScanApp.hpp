#pragma once
// WpsScanApp — listen for beacons on each channel, parse the
// vendor-specific WPS information element (Microsoft OUI 00:50:F2
// type 0x04), and list APs that advertise WPS. Helps identify
// targets for further (offline) WPS-PIN analysis on a real lab.
//
// Doesn't TX; pure RX. No deauth needed.
#include "yui/app/App.hpp"
#include "yui/hal/IWifiMonitor.hpp"
#include "yui/proto/Dot11.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class WpsScanApp : public App {
public:
  static constexpr size_t kMaxAps = 32;
  static constexpr uint32_t kHopMs = 400;
  static constexpr uint8_t  kFirstChannel = 1;
  static constexpr uint8_t  kLastChannel  = 13;

  struct Entry {
    char    ssid[33]      = {0};
    uint8_t bssid[6]      = {0};
    uint8_t channel       = 0;
    int8_t  rssi          = 0;
    bool    wps           = false;
  };

  explicit WpsScanApp(IWifiMonitor& mon) : mon_(mon) {}

  const char* name() const override { return "WPS Scan"; }
  Category    category() const override { return Category::WiFi; }

  void on_enter(Hal& hal) override {
    hal_         = &hal;
    count_       = 0;
    cursor_      = 0;
    cur_channel_ = kFirstChannel;
    last_hop_ms_ = 0;
    WifiFilter f{}; f.mgmt = true; f.data = false; f.ctrl = false;
    mon_.set_callback(&rx_thunk_, this);
    mon_.start(f, cur_channel_);
  }

  void on_exit() override { mon_.stop(); }

  void tick(uint32_t now_ms) override {
    if (!mon_.running()) return;
    if (now_ms - last_hop_ms_ < kHopMs && last_hop_ms_ != 0) return;
    last_hop_ms_ = now_ms == 0 ? 1 : now_ms;
    cur_channel_ = (cur_channel_ < kLastChannel) ? (cur_channel_ + 1)
                                                 : kFirstChannel;
    mon_.set_channel(cur_channel_);
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Up   && cursor_ > 0)         --cursor_;
    if (k.key == Key::Down && cursor_ + 1 < count_) ++cursor_;
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    char title[32];
    int wps_count = 0;
    for (size_t i = 0; i < count_; ++i) if (entries_[i].wps) ++wps_count;
    std::snprintf(title, sizeof(title), "WPS  ch%u  %d/%u",
                  static_cast<unsigned>(cur_channel_),
                  wps_count, static_cast<unsigned>(count_));
    ui::Chrome::header(d, title);

    if (count_ == 0) {
      d.draw_text(8, 40, "Scanning...", kJapanRedDark, kWhite);
      d.flush();
      return;
    }
    const size_t window = 6;
    const size_t start = (cursor_ >= window) ? (cursor_ - window + 1) : 0;
    char line[40];
    for (size_t i = start; i < count_ && i < start + window; ++i) {
      const Entry& e = entries_[i];
      const int y = 22 + static_cast<int>(i - start) * 16;
      const bool sel = (i == cursor_);
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : (e.wps ? kJapanRedBright : kBlack);
      if (sel) d.fill_rect({0, y - 2, d.width(), 16}, bg);
      std::snprintf(line, sizeof(line), "%c %-18s ch%u",
                    e.wps ? 'W' : ' ',
                    e.ssid[0] ? e.ssid : "(hidden)",
                    static_cast<unsigned>(e.channel));
      d.draw_text(4, y + 3, line, fg, bg);
    }
    d.flush();
  }

  // Test hooks
  size_t       count()      const { return count_; }
  const Entry& entry_at(size_t i) const { return entries_[i]; }

private:
  static void rx_thunk_(void* ctx, const uint8_t* frame, size_t len,
                        const WifiRxMeta& meta) {
    static_cast<WpsScanApp*>(ctx)->on_rx_(frame, len, meta);
  }

  void on_rx_(const uint8_t* frame, size_t len, const WifiRxMeta& meta) {
    if (len < 24) return;
    const uint8_t st = dot11::fc_subtype(frame);
    if (dot11::fc_type(frame) != dot11::kTypeMgmt) return;
    if (st != dot11::kSubBeacon && st != dot11::kSubProbeResp) return;

    const uint8_t* bssid = dot11::addr3(frame);
    char ssid[33] = {0};
    dot11::extract_ssid(frame, len, ssid, sizeof(ssid));
    const bool wps = dot11::has_wps_ie(frame, len);

    // Find or insert
    for (size_t i = 0; i < count_; ++i) {
      if (std::memcmp(entries_[i].bssid, bssid, 6) == 0) {
        if (wps) entries_[i].wps = true;     // sticky: once seen, stays
        entries_[i].rssi = meta.rssi;
        entries_[i].channel = (meta.channel ? meta.channel : cur_channel_);
        return;
      }
    }
    if (count_ >= kMaxAps) return;
    Entry& e = entries_[count_++];
    std::strncpy(e.ssid, ssid, sizeof(e.ssid) - 1);
    e.ssid[sizeof(e.ssid) - 1] = '\0';
    std::memcpy(e.bssid, bssid, 6);
    e.channel = (meta.channel ? meta.channel : cur_channel_);
    e.rssi    = meta.rssi;
    e.wps     = wps;
  }

  IWifiMonitor& mon_;
  Hal*          hal_           = nullptr;
  Entry         entries_[kMaxAps];
  size_t        count_         = 0;
  size_t        cursor_        = 0;
  uint8_t       cur_channel_   = kFirstChannel;
  uint32_t      last_hop_ms_   = 0;
};

}  // namespace yui

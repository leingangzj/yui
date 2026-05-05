#pragma once
// WifiProbeApp — passive collection of 802.11 probe-request frames.
// Each unique (SSID, source-MAC) pair is recorded once; a counter
// tracks how many times each SSID has been requested. Useful for
// fingerprinting nearby clients (the SSIDs they remember).
//
// Channel hopping: every kHopMs ms, advance to the next channel in
// the 1..13 range. (5 GHz needs Pineapple — see PINEAPPLE_API.md.)
//
// Pure RX, totally legal — passive monitor only.
#include "yui/app/App.hpp"
#include "yui/hal/IWifiMonitor.hpp"
#include "yui/proto/Dot11.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class WifiProbeApp : public App {
public:
  static constexpr size_t kMaxEntries = 64;
  static constexpr uint32_t kHopMs = 250;
  static constexpr uint8_t  kFirstChannel = 1;
  static constexpr uint8_t  kLastChannel  = 13;

  struct Entry {
    char    ssid[33] = {0};
    uint8_t client_mac[6] = {0};
    uint32_t count   = 0;
  };

  explicit WifiProbeApp(IWifiMonitor& mon) : mon_(mon) {}

  const char* name() const override { return "Probes"; }
  Category    category() const override { return Category::WiFi; }
  const assets::IconRef* icon() const override { return &assets::icons::kScan(); }

  void on_enter(Hal& hal) override {
    hal_   = &hal;
    count_ = 0;
    cursor_ = 0;
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
    if (k.key == Key::Up   && cursor_ > 0)             --cursor_;
    if (k.key == Key::Down && cursor_ + 1 < count_)    ++cursor_;
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    char title[32];
    std::snprintf(title, sizeof(title), "Probes  ch%u  %u",
                  static_cast<unsigned>(cur_channel_),
                  static_cast<unsigned>(count_));
    ui::Chrome::header(d, title);

    if (count_ == 0) {
      d.draw_text(8, 40, "Listening...", ui::kAccentDark, ui::kSurface);
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
      const Color bg = sel ? ui::kAccent : ui::kSurface;
      const Color fg = sel ? ui::kSurface    : ui::kOnSurface;
      if (sel) d.fill_rect({0, y - 2, d.width(), 16}, bg);
      std::snprintf(line, sizeof(line), "%-20s %u",
                    e.ssid[0] ? e.ssid : "(broadcast)",
                    static_cast<unsigned>(e.count));
      d.draw_text(4, y + 3, line, fg, bg);
    }
    d.flush();
  }

  // Test hooks
  size_t  entry_count() const { return count_; }
  const Entry& entry_at(size_t i) const { return entries_[i]; }
  uint8_t channel() const { return cur_channel_; }

private:
  static void rx_thunk_(void* ctx, const uint8_t* frame, size_t len,
                        const WifiRxMeta&) {
    static_cast<WifiProbeApp*>(ctx)->on_rx_(frame, len);
  }

  void on_rx_(const uint8_t* frame, size_t len) {
    if (!dot11::is_probe_request(frame, len)) return;
    char ssid[33] = {0};
    dot11::extract_ssid(frame, len, ssid, sizeof(ssid));
    const uint8_t* sa = dot11::addr2(frame);
    upsert_(ssid, sa);
  }

  void upsert_(const char* ssid, const uint8_t* mac) {
    for (size_t i = 0; i < count_; ++i) {
      if (std::strcmp(entries_[i].ssid, ssid) == 0 &&
          std::memcmp(entries_[i].client_mac, mac, 6) == 0) {
        ++entries_[i].count;
        return;
      }
    }
    if (count_ >= kMaxEntries) return;   // drop new entries when full
    Entry& e = entries_[count_++];
    std::strncpy(e.ssid, ssid, sizeof(e.ssid) - 1);
    e.ssid[sizeof(e.ssid) - 1] = '\0';
    std::memcpy(e.client_mac, mac, 6);
    e.count = 1;
  }

  IWifiMonitor&  mon_;
  Hal*           hal_           = nullptr;
  Entry          entries_[kMaxEntries];
  size_t         count_         = 0;
  size_t         cursor_        = 0;
  uint8_t        cur_channel_   = kFirstChannel;
  uint32_t       last_hop_ms_   = 0;
};

}  // namespace yui

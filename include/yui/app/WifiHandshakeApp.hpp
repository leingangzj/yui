#pragma once
// WifiHandshakeApp — passive WPA handshake capture. Listens for EAPOL
// data frames + the surrounding mgmt frames (auth/assoc/deauth) and
// streams every captured frame to a libpcap file on SD via IPcap.
//
// PASSIVE ONLY: per the protocol audit, native esp_wifi_80211_tx
// excludes deauth, and the patched-libnet workaround is fragile. We
// don't provoke handshakes — we wait for natural reassoc events.
// Active deauth is the Pineapple's job.
//
// Channel hopping behavior matches WifiProbeApp.
#include "yui/app/App.hpp"
#include "yui/hal/IWifiMonitor.hpp"
#include "yui/hal/IPcap.hpp"
#include "yui/hal/IClock.hpp"
#include "yui/proto/Dot11.hpp"
#include "yui/proto/Eapol.hpp"
#include "yui/proto/Pcap.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class WifiHandshakeApp : public App {
public:
  enum class Mode { FourWay, PmkidOnly };

  static constexpr uint32_t kHopMs        = 250;
  static constexpr uint8_t  kFirstChannel = 1;
  static constexpr uint8_t  kLastChannel  = 13;

  WifiHandshakeApp(IWifiMonitor& mon, IPcap& pcap, IClock& clock)
      : mon_(mon), pcap_(pcap), clock_(clock) {}

  const char* name() const override { return "Handshake"; }
  Category    category() const override { return Category::WiFi; }
  const assets::IconRef* icon() const override { return &assets::icons::kCapture(); }

  void on_enter(Hal& hal) override {
    hal_     = &hal;
    pkt_total_  = 0;
    eapol_seen_ = 0;
    pmkid_seen_ = 0;
    cur_channel_ = kFirstChannel;
    last_hop_ms_ = 0;
    file_open_   = false;
    last_bssid_[0] = '\0';
    open_capture_();
    if (!file_open_) return;
    WifiFilter f{}; f.mgmt = true; f.data = true;
    mon_.set_callback(&rx_thunk_, this);
    mon_.start(f, cur_channel_);
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Tab) {
      mode_ = (mode_ == Mode::FourWay) ? Mode::PmkidOnly : Mode::FourWay;
    }
  }

  void on_exit() override {
    mon_.stop();
    if (file_open_) {
      pcap_.close();
      file_open_ = false;
    }
  }

  void tick(uint32_t now_ms) override {
    if (!mon_.running()) return;
    if (now_ms - last_hop_ms_ < kHopMs && last_hop_ms_ != 0) return;
    last_hop_ms_ = now_ms == 0 ? 1 : now_ms;
    cur_channel_ = (cur_channel_ < kLastChannel) ? (cur_channel_ + 1)
                                                 : kFirstChannel;
    mon_.set_channel(cur_channel_);
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    char ch[16];
    std::snprintf(ch, sizeof(ch), "ch%u %s",
                  static_cast<unsigned>(cur_channel_),
                  mode_ == Mode::PmkidOnly ? "PMKID" : "4-way");
    ui::Chrome::header(d, "Handshake", ch);

    int y = ui::kBodyTopY;
    if (!file_open_) {
      d.draw_text(ui::kBodyPadX, y, "SD write failed",
                  ui::kWarn, ui::kSurface);
      y += ui::kBodyLineH;
    }
    char val[24];
    std::snprintf(val, sizeof(val), "%u", static_cast<unsigned>(pkt_total_));
    ui::Chrome::stat(d, (y - ui::kBodyTopY) / ui::kBodyLineH, "Frames", val);
    y += ui::kBodyLineH;

    std::snprintf(val, sizeof(val), "%u", static_cast<unsigned>(eapol_seen_));
    ui::Chrome::stat(d, (y - ui::kBodyTopY) / ui::kBodyLineH, "EAPOL", val);
    y += ui::kBodyLineH;

    std::snprintf(val, sizeof(val), "%u", static_cast<unsigned>(pmkid_seen_));
    ui::Chrome::stat(d, (y - ui::kBodyTopY) / ui::kBodyLineH, "PMKID", val);
    y += ui::kBodyLineH;

    if (last_bssid_[0]) {
      ui::Chrome::stat(d, (y - ui::kBodyTopY) / ui::kBodyLineH,
                       "Last AP", last_bssid_);
      y += ui::kBodyLineH;
    }

    std::snprintf(val, sizeof(val), "%llu KB",
                  static_cast<unsigned long long>(pcap_.bytes_written() / 1024));
    ui::Chrome::stat(d, (y - ui::kBodyTopY) / ui::kBodyLineH, "File", val);

    ui::Chrome::footer(d, "Tab:mode  Esc:back");
    d.flush();
  }

  // Test hooks
  size_t pkt_total()    const { return pkt_total_; }
  size_t eapol_seen()   const { return eapol_seen_; }
  size_t pmkid_seen()   const { return pmkid_seen_; }
  bool   file_open()    const { return file_open_; }
  uint8_t channel()     const { return cur_channel_; }
  const char* last_bssid() const { return last_bssid_; }
  Mode    mode()        const { return mode_; }
  void    set_mode(Mode m) { mode_ = m; }
  // Inject a frame as if the radio received it. Used by tests so the
  // PMKID-extraction path can be exercised without a live monitor.
  void inject_for_test(const uint8_t* frame, std::size_t len) {
    on_rx_(frame, len);
  }

private:
  static void rx_thunk_(void* ctx, const uint8_t* frame, size_t len,
                        const WifiRxMeta&) {
    static_cast<WifiHandshakeApp*>(ctx)->on_rx_(frame, len);
  }

  void on_rx_(const uint8_t* frame, size_t len) {
    if (len < 24) return;
    if (file_open_) {
      const uint64_t ts_us = static_cast<uint64_t>(clock_.millis()) * 1000ULL;
      pcap_.write_packet(frame, len, ts_us);
    }
    ++pkt_total_;
    if (dot11::is_eapol_data(frame, len)) {
      ++eapol_seen_;
      dot11::format_mac(dot11::addr3(frame), last_bssid_);
      // PMKID extraction: the EAPOL body sits right after the MAC
      // header + LLC/SNAP. For non-QoS that's offset 24 + 8 = 32.
      const std::size_t hdr = (dot11::fc_subtype(frame) & 0x08) ? 26 : 24;
      const std::size_t off = hdr + 8;
      if (len > off) {
        uint8_t pmkid[16];
        if (eapol::extract_pmkid(frame + off, len - off, pmkid)) {
          ++pmkid_seen_;
        }
      }
    }
  }

  void open_capture_() {
    char path[64];
    const uint64_t ts = static_cast<uint64_t>(clock_.epoch_seconds());
    std::snprintf(path, sizeof(path), "/handshakes/cap-%llu.pcap",
                  static_cast<unsigned long long>(ts));
    file_open_ = pcap_.open(path, PcapLinkType::Ieee80211, 65535);
  }

  IWifiMonitor&  mon_;
  IPcap&         pcap_;
  IClock&        clock_;
  Hal*           hal_         = nullptr;
  Mode           mode_        = Mode::FourWay;
  size_t         pkt_total_   = 0;
  size_t         eapol_seen_  = 0;
  size_t         pmkid_seen_  = 0;
  uint8_t        cur_channel_ = kFirstChannel;
  uint32_t       last_hop_ms_ = 0;
  bool           file_open_   = false;
  char           last_bssid_[18] = {0};
};

}  // namespace yui

#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32WifiMonitor — promiscuous-mode WiFi RX + raw 802.11 TX.
//
// RX path: esp_wifi_set_promiscuous_rx_cb dispatches each captured
// frame to a static callback that fans out to the registered
// IWifiMonitor consumer.
//
// TX path: esp_wifi_80211_tx natively supports beacon / probe /
// action / non-QoS data frames. Deauth/disassoc are blocked by
// Espressif's libnet80211; we attempt them via the same call and
// surface failure (false return) to the caller. The "patched-libnet"
// workaround used by Bruce/Marauder requires linker-level overriding
// of ieee80211_freedom_output via -zmuldefs and a vendored
// libnet80211.a — which is fragile across IDF upgrades. Yui keeps
// the call-site in place but does not ship the patched binary in
// v0.3; if/when Zac wants real TX of deauth on hardware, drop the
// patched archive into board/lib_extra and add the linker flag.
#include "yui/hal/IWifiMonitor.hpp"
#include <WiFi.h>
extern "C" {
#include "esp_wifi.h"
}

namespace yui {

class Esp32WifiMonitor : public IWifiMonitor {
public:
  static Esp32WifiMonitor* instance() { return self_; }

  Esp32WifiMonitor() { self_ = this; }
  ~Esp32WifiMonitor() override { if (self_ == this) self_ = nullptr; }

  bool start(const WifiFilter& filter, uint8_t channel) override {
    if (running_) stop();
    // Bring up WiFi in NULL mode (no STA, no AP) for pure monitor.
    WiFi.mode(WIFI_OFF);
    delay(50);
    if (esp_wifi_set_mode(WIFI_MODE_NULL) != ESP_OK) return false;
    if (esp_wifi_start() != ESP_OK) return false;

    wifi_promiscuous_filter_t f = {};
    if (filter.mgmt) f.filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT;
    if (filter.ctrl) f.filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL;
    if (filter.data) f.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA;
    if (filter.misc) f.filter_mask |= WIFI_PROMIS_FILTER_MASK_MISC;
    esp_wifi_set_promiscuous_filter(&f);

    esp_wifi_set_promiscuous_rx_cb(&Esp32WifiMonitor::rx_thunk_);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    channel_ = channel;
    running_ = true;
    return true;
  }

  void stop() override {
    if (!running_) return;
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    running_ = false;
  }

  bool set_channel(uint8_t ch) override {
    if (!running_) return false;
    if (esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE) != ESP_OK) return false;
    channel_ = ch;
    return true;
  }

  void set_callback(WifiRxCallback cb, void* ctx) override {
    cb_  = cb;
    ctx_ = ctx;
  }

  bool running() const override { return running_; }
  uint8_t channel() const override { return channel_; }

  bool tx_raw(const uint8_t* frame, size_t len) override {
    if (!running_ || !frame || len < 24 || len > 1500) return false;
    // WIFI_IF_STA when STA mode is up; in MODE_NULL we use WIFI_IF_STA
    // anyway since it's the radio context. Some Espressif firmwares
    // require an AP up too — test-driven on hardware.
    return esp_wifi_80211_tx(WIFI_IF_STA, frame,
                             static_cast<int>(len),
                             /*en_sys_seq=*/false) == ESP_OK;
  }

private:
  static void rx_thunk_(void* buf, wifi_promiscuous_pkt_type_t type) {
    auto* self = self_;
    if (!self || !self->cb_) return;
    auto* pkt = static_cast<wifi_promiscuous_pkt_t*>(buf);
    WifiRxMeta meta;
    meta.rssi    = pkt->rx_ctrl.rssi;
    meta.channel = pkt->rx_ctrl.channel;
    switch (type) {
      case WIFI_PKT_MGMT: meta.type = WifiPktType::Management; break;
      case WIFI_PKT_CTRL: meta.type = WifiPktType::Control;    break;
      case WIFI_PKT_DATA: meta.type = WifiPktType::Data;       break;
      default:            meta.type = WifiPktType::Misc;       break;
    }
    self->cb_(self->ctx_, pkt->payload, pkt->rx_ctrl.sig_len, meta);
  }

  static Esp32WifiMonitor* self_;
  WifiRxCallback cb_      = nullptr;
  void*          ctx_     = nullptr;
  uint8_t        channel_ = 0;
  bool           running_ = false;
};

inline Esp32WifiMonitor* Esp32WifiMonitor::self_ = nullptr;

}  // namespace yui
#endif

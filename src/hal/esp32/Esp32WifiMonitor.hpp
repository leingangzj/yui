#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32WifiMonitor — wraps esp_wifi_set_promiscuous_rx_cb. Real impl
// belongs in v0.2 Track C — this stub is just the shape so main.cpp
// can construct one. Actual init sequence per
// docs/protocols/ESP_WIFI_AND_PCAP.md §1:
//   esp_wifi_init → set_mode(WIFI_MODE_NULL) → start →
//   set_promiscuous_filter → set_promiscuous_rx_cb → set_promiscuous(true)
//   set_channel(N, NONE)
#include "yui/hal/IWifiMonitor.hpp"

namespace yui {

class Esp32WifiMonitor : public IWifiMonitor {
public:
  bool start(const WifiFilter& /*filter*/, uint8_t /*channel*/) override {
    return false;
  }
  void stop() override { running_ = false; }
  bool set_channel(uint8_t /*ch*/) override { return false; }
  void set_callback(WifiRxCallback cb, void* ctx) override {
    cb_ = cb; ctx_ = ctx;
  }
  bool running() const override { return running_; }
  uint8_t channel() const override { return channel_; }

  // STUB until v0.3: real impl will be esp_wifi_80211_tx for beacon /
  // probe / action / non-QoS data. Deauth/disassoc require patched
  // libnet80211.a (zmuldefs override of ieee80211_freedom_output).
  bool tx_raw(const uint8_t* /*frame*/, size_t /*len*/) override {
    return false;
  }

private:
  WifiRxCallback cb_      = nullptr;
  void*          ctx_     = nullptr;
  uint8_t        channel_ = 0;
  bool           running_ = false;
};

}  // namespace yui
#endif

#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32WifiAp — wraps WiFi.softAP() + WiFiUDP (DNS responder) +
// WiFiServer (HTTP captive portal). STUB until v0.3 hardware bring-up.
//
// Real impl per docs/protocols/PINEAPPLE_API.md (we already do HTTP
// client) plus a tiny inline DNS server that crafts an A-record
// response packet pointing at WiFi.softAPIP() for any incoming
// query.
#include "yui/hal/IWifiAp.hpp"

namespace yui {

class Esp32WifiAp : public IWifiAp {
public:
  bool start_open(const char* /*ssid*/, uint8_t /*channel*/) override { return false; }
  void stop() override { active_ = false; }
  bool active() const override { return active_; }
  size_t client_count() const override { return 0; }
  bool start_captive(const char* /*html*/, CaptiveFormCb /*cb*/, void* /*ctx*/) override {
    return false;
  }
  size_t dns_queries() const override { return 0; }
  size_t http_requests() const override { return 0; }

private:
  bool active_ = false;
};

}  // namespace yui
#endif

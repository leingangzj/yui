#pragma once
// IWifiAp — Cardputer runs as a SoftAP. Used by CaptivePortalApp to
// stand up an open or fake-secured AP, plus an integrated DNS+HTTP
// server that returns a captive-portal page for any DNS query.
//
// On ESP32 this wraps WiFi.softAP() + WiFiUDP (DNS) + WiFiServer
// (HTTP). Native fake records all calls for tests.
#include <cstdint>
#include <cstddef>

namespace yui {

// Fired when a connecting client posts something to the captive
// portal HTML form. `body` is the urlencoded form payload.
using CaptiveFormCb = void (*)(void* ctx, const char* peer_ip,
                               const char* form_body);

class IWifiAp {
public:
  virtual ~IWifiAp() = default;

  // Start an open AP broadcasting `ssid` on `channel`. Begins DHCP.
  virtual bool start_open(const char* ssid, uint8_t channel = 6) = 0;

  // Stop AP, DNS, and HTTP. Idempotent.
  virtual void stop() = 0;

  virtual bool active() const = 0;
  virtual size_t client_count() const = 0;

  // Begin captive-portal mode: DNS server on UDP/53 returns our IP for
  // any A query; HTTP server on TCP/80 serves `html` to any GET and
  // calls `cb` with form bodies on POST. Both run after start_open().
  virtual bool start_captive(const char* html,
                             CaptiveFormCb cb, void* ctx) = 0;

  virtual size_t dns_queries() const = 0;
  virtual size_t http_requests() const = 0;
};

}  // namespace yui

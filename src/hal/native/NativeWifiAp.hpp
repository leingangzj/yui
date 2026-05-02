#pragma once
#include "yui/hal/IWifiAp.hpp"
#include <cstring>
#include <string>

namespace yui {

class FakeWifiAp : public IWifiAp {
public:
  bool start_open(const char* ssid, uint8_t channel) override {
    ssid_ = ssid ? ssid : "";
    channel_ = channel;
    active_ = true;
    return true;
  }
  void stop() override {
    active_ = false; captive_ = false;
    cb_ = nullptr; ctx_ = nullptr;
  }
  bool active() const override { return active_; }
  size_t client_count() const override { return clients_; }

  bool start_captive(const char* html, CaptiveFormCb cb, void* ctx) override {
    if (!active_) return false;
    html_ = html ? html : "";
    cb_ = cb; ctx_ = ctx;
    captive_ = true;
    return true;
  }

  size_t dns_queries() const override { return dns_count_; }
  size_t http_requests() const override { return http_count_; }

  // ─── Test fixture ─────────────────────────────────────────────────
  // Simulate clients connecting / DNS queries / form submissions.
  void simulate_client_join() { ++clients_; }
  void simulate_client_leave() { if (clients_) --clients_; }
  void simulate_dns_query()    { ++dns_count_; }
  void simulate_http_get()     { ++http_count_; }
  void simulate_form_submit(const char* peer_ip, const char* body) {
    ++http_count_;
    if (cb_) cb_(ctx_, peer_ip, body);
  }

  const std::string& ssid()      const { return ssid_; }
  const std::string& html()      const { return html_; }
  uint8_t            channel()   const { return channel_; }
  bool               captive()   const { return captive_; }

private:
  std::string ssid_;
  std::string html_;
  uint8_t     channel_     = 0;
  bool        active_      = false;
  bool        captive_     = false;
  size_t      clients_     = 0;
  size_t      dns_count_   = 0;
  size_t      http_count_  = 0;
  CaptiveFormCb cb_  = nullptr;
  void*         ctx_ = nullptr;
};

}  // namespace yui

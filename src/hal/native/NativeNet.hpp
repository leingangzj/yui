#pragma once
#include "yui/hal/INet.hpp"
#include <vector>
#include <cstring>

namespace yui {

// Test-driver INet backend. Tests pre-populate results and decide when scans
// "complete" via the simulate_*_done() helpers.
class FakeNet : public INet {
public:
  // ─── WiFi scan ───────────────────────────────────────────────────────
  void set_wifi_results(std::vector<WifiAp> aps) { wifi_aps_ = std::move(aps); }
  void simulate_wifi_done() { wifi_ready_ = true; }

  bool wifi_scan_start() override {
    wifi_ready_ = wifi_immediate_;
    ++wifi_starts_;
    return true;
  }
  bool wifi_scan_ready() override { return wifi_ready_; }
  size_t wifi_count() const override { return wifi_aps_.size(); }
  const WifiAp& wifi_at(size_t i) const override { return wifi_aps_[i]; }

  void set_wifi_immediate(bool v) { wifi_immediate_ = v; }
  int  wifi_starts() const { return wifi_starts_; }

  // ─── WiFi connection ─────────────────────────────────────────────────
  bool wifi_connect(const char* ssid, const char* pass) override {
    if (!ssid || ssid[0] == 0) return false;
    std::strncpy(last_ssid_, ssid, sizeof(last_ssid_) - 1);
    last_ssid_[sizeof(last_ssid_) - 1] = 0;
    std::strncpy(last_pass_, pass ? pass : "", sizeof(last_pass_) - 1);
    last_pass_[sizeof(last_pass_) - 1] = 0;
    state_ = wifi_connect_immediate_ ? WifiState::Connected : WifiState::Connecting;
    if (state_ == WifiState::Connected) {
      std::strncpy(ip_, "192.168.1.42", sizeof(ip_));
    }
    ++connects_;
    return true;
  }
  void wifi_disconnect() override {
    state_ = WifiState::Idle;
    std::strncpy(ip_, "0.0.0.0", sizeof(ip_));
  }
  WifiState   wifi_state() override { return state_; }
  const char* wifi_ip()    override { return ip_; }

  void simulate_wifi_connected(const char* ip = "192.168.1.42") {
    state_ = WifiState::Connected;
    std::strncpy(ip_, ip ? ip : "0.0.0.0", sizeof(ip_) - 1);
    ip_[sizeof(ip_) - 1] = 0;
  }
  void simulate_wifi_failed() { state_ = WifiState::Failed; }
  void set_wifi_connect_immediate(bool v) { wifi_connect_immediate_ = v; }
  int  connects() const { return connects_; }
  const char* last_ssid() const { return last_ssid_; }
  const char* last_pass() const { return last_pass_; }

  // ─── NTP ─────────────────────────────────────────────────────────────
  bool ntp_sync(const char* server, const char* tz) override {
    if (state_ != WifiState::Connected) return false;
    std::strncpy(last_ntp_server_, server ? server : "", sizeof(last_ntp_server_) - 1);
    std::strncpy(last_ntp_tz_,     tz     ? tz     : "", sizeof(last_ntp_tz_) - 1);
    ++ntp_calls_;
    return true;
  }
  int ntp_calls() const { return ntp_calls_; }
  const char* last_ntp_server() const { return last_ntp_server_; }
  const char* last_ntp_tz()     const { return last_ntp_tz_; }

  // ─── BLE ─────────────────────────────────────────────────────────────
  void set_ble_results(std::vector<BleDevice> devs) { ble_devs_ = std::move(devs); }
  void simulate_ble_done() { ble_ready_ = true; }

  bool ble_scan_start(uint32_t /*duration_ms*/) override {
    ble_ready_ = ble_immediate_;
    ++ble_starts_;
    return true;
  }
  bool ble_scan_ready() override { return ble_ready_; }
  size_t ble_count() const override { return ble_devs_.size(); }
  const BleDevice& ble_at(size_t i) const override { return ble_devs_[i]; }

  void set_ble_immediate(bool v) { ble_immediate_ = v; }
  int  ble_starts() const { return ble_starts_; }

private:
  std::vector<WifiAp>    wifi_aps_;
  std::vector<BleDevice> ble_devs_;
  bool wifi_ready_     = false;
  bool wifi_immediate_ = true;
  int  wifi_starts_    = 0;
  bool ble_ready_      = false;
  bool ble_immediate_  = true;
  int  ble_starts_     = 0;

  WifiState state_ = WifiState::Idle;
  char ip_[16]        = "0.0.0.0";
  char last_ssid_[33] = {0};
  char last_pass_[65] = {0};
  bool wifi_connect_immediate_ = true;
  int  connects_ = 0;

  char last_ntp_server_[64] = {0};
  char last_ntp_tz_[40]     = {0};
  int  ntp_calls_ = 0;
};

}  // namespace yui

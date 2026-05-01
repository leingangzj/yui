#pragma once
#include "yui/hal/INet.hpp"
#include <vector>
#include <cstring>

namespace yui {

// Test-driver INet backend. Tests pre-populate results and decide when scans
// "complete" via the simulate_*_done() helpers.
class FakeNet : public INet {
public:
  // ─── WiFi ────────────────────────────────────────────────────────────
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

  // If true, scan_start() also marks the result as ready immediately.
  void set_wifi_immediate(bool v) { wifi_immediate_ = v; }
  int  wifi_starts() const { return wifi_starts_; }

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
  std::vector<WifiAp>   wifi_aps_;
  std::vector<BleDevice> ble_devs_;
  bool wifi_ready_     = false;
  bool wifi_immediate_ = true;
  int  wifi_starts_    = 0;
  bool ble_ready_      = false;
  bool ble_immediate_  = true;
  int  ble_starts_     = 0;
};

}  // namespace yui

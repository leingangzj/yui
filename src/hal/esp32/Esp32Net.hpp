#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/INet.hpp"
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <vector>
#include <cstring>
#include <cstdio>

namespace yui {

class Esp32Net : public INet {
public:
  // ─── WiFi ────────────────────────────────────────────────────────────
  bool wifi_scan_start() override {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, true);
    delay(50);
    WiFi.scanDelete();
    return WiFi.scanNetworks(/*async=*/true) >= 0;
  }

  bool wifi_scan_ready() override {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING || n == WIFI_SCAN_FAILED) return false;
    if (!wifi_cached_) cache_wifi_(n);
    return true;
  }

  size_t wifi_count() const override { return wifi_aps_.size(); }
  const WifiAp& wifi_at(size_t i) const override { return wifi_aps_[i]; }

  // ─── BLE ─────────────────────────────────────────────────────────────
  bool ble_scan_start(uint32_t duration_ms) override {
    if (!ble_inited_) {
      BLEDevice::init("");
      ble_inited_ = true;
    }
    auto* scan = BLEDevice::getScan();
    scan->setActiveScan(true);
    scan->setInterval(0x50);
    scan->setWindow(0x30);
    ble_results_ = scan->start(static_cast<uint32_t>(duration_ms / 1000), false);
    cache_ble_();
    ble_done_ = true;
    return true;
  }

  bool ble_scan_ready() override { return ble_done_; }
  size_t ble_count() const override { return ble_devs_.size(); }
  const BleDevice& ble_at(size_t i) const override { return ble_devs_[i]; }

private:
  void cache_wifi_(int n) {
    wifi_aps_.clear();
    if (n <= 0) { wifi_cached_ = true; return; }
    wifi_aps_.reserve(n);
    for (int i = 0; i < n; ++i) {
      WifiAp ap{};
      const String ssid = WiFi.SSID(i);
      std::strncpy(ap.ssid, ssid.c_str(), sizeof(ap.ssid) - 1);
      const String bssid = WiFi.BSSIDstr(i);
      std::strncpy(ap.bssid, bssid.c_str(), sizeof(ap.bssid) - 1);
      ap.rssi    = static_cast<int8_t>(WiFi.RSSI(i));
      ap.channel = static_cast<uint8_t>(WiFi.channel(i));
      ap.secured = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
      wifi_aps_.push_back(ap);
    }
    wifi_cached_ = true;
  }

  void cache_ble_() {
    ble_devs_.clear();
    const int n = ble_results_.getCount();
    if (n <= 0) return;
    ble_devs_.reserve(n);
    for (int i = 0; i < n; ++i) {
      auto adv = ble_results_.getDevice(i);
      BleDevice d{};
      const std::string name = adv.getName();
      std::strncpy(d.name, name.c_str(), sizeof(d.name) - 1);
      std::strncpy(d.addr, adv.getAddress().toString().c_str(), sizeof(d.addr) - 1);
      d.rssi = static_cast<int8_t>(adv.getRSSI());
      ble_devs_.push_back(d);
    }
  }

  std::vector<WifiAp>    wifi_aps_;
  std::vector<BleDevice> ble_devs_;
  BLEScanResults         ble_results_;
  bool wifi_cached_ = false;
  bool ble_inited_  = false;
  bool ble_done_    = false;
};

}  // namespace yui
#endif

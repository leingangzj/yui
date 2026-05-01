#pragma once
#include <cstdint>
#include <cstddef>

namespace yui {

struct WifiAp {
  char    ssid[33] = {0};   // 32-byte SSID + NUL
  char    bssid[18] = {0};  // "AA:BB:CC:DD:EE:FF" + NUL
  int8_t  rssi    = 0;      // dBm
  uint8_t channel = 0;
  bool    secured = false;
};

struct BleDevice {
  char    name[33] = {0};
  char    addr[18] = {0};
  int8_t  rssi    = 0;
};

class INet {
public:
  virtual ~INet() = default;

  // ─── WiFi scanning ────────────────────────────────────────────────────
  // Returns true if scan kicked off; previous results are discarded.
  virtual bool   wifi_scan_start() = 0;
  // True once results are available. Implementations may poll-cache.
  virtual bool   wifi_scan_ready() = 0;
  virtual size_t wifi_count() const = 0;
  // i must be < wifi_count(). Result is borrowed; copy if you need to retain.
  virtual const WifiAp& wifi_at(size_t i) const = 0;

  // ─── BLE scanning ─────────────────────────────────────────────────────
  virtual bool   ble_scan_start(uint32_t duration_ms) = 0;
  virtual bool   ble_scan_ready() = 0;
  virtual size_t ble_count() const = 0;
  virtual const BleDevice& ble_at(size_t i) const = 0;
};

}  // namespace yui

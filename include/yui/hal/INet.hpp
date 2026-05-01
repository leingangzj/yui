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

enum class WifiState : uint8_t {
  Idle,        // not associated
  Connecting,  // join in progress
  Connected,   // associated + IP
  Failed,      // last attempt failed
};

class INet {
public:
  virtual ~INet() = default;

  // ─── WiFi scanning ────────────────────────────────────────────────────
  virtual bool   wifi_scan_start() = 0;
  virtual bool   wifi_scan_ready() = 0;
  virtual size_t wifi_count() const = 0;
  virtual const WifiAp& wifi_at(size_t i) const = 0;

  // ─── WiFi connection ─────────────────────────────────────────────────
  // Kick off association. Returns false on bad args; success is reported
  // asynchronously via wifi_state(). pass may be empty for open networks.
  virtual bool      wifi_connect(const char* ssid, const char* pass) = 0;
  virtual void      wifi_disconnect() = 0;
  virtual WifiState wifi_state() = 0;
  // Dotted-quad IPv4 string ("0.0.0.0" if not connected). Borrowed.
  virtual const char* wifi_ip() = 0;

  // ─── NTP / time sync ──────────────────────────────────────────────────
  // Kick configTzTime; system clock fills in asynchronously. tz is a POSIX
  // TZ string ("UTC0", "EST5EDT,M3.2.0,M11.1.0", ...). Requires Connected.
  virtual bool ntp_sync(const char* server, const char* tz) = 0;

  // ─── BLE scanning ─────────────────────────────────────────────────────
  virtual bool   ble_scan_start(uint32_t duration_ms) = 0;
  virtual bool   ble_scan_ready() = 0;
  virtual size_t ble_count() const = 0;
  virtual const BleDevice& ble_at(size_t i) const = 0;
};

}  // namespace yui

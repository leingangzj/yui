#pragma once
// IWifiMonitor — 802.11 promiscuous-mode RX. The ESP32 backend wires
// esp_wifi_set_promiscuous_rx_cb; the native backend lets tests inject
// frames synchronously.
//
// See docs/protocols/ESP_WIFI_AND_PCAP.md for the init sequence and
// filter masks. This HAL deliberately exposes a tiny surface — apps
// receive each frame via the registered callback.
#include <cstdint>
#include <cstddef>

namespace yui {

enum class WifiPktType : uint8_t {
  Management,
  Control,
  Data,
  Misc,
};

struct WifiRxMeta {
  int8_t  rssi    = 0;
  uint8_t channel = 0;
  WifiPktType type = WifiPktType::Misc;
};

// Bitmask of which frame types to deliver. Matches ESP-IDF's
// WIFI_PROMIS_FILTER_MASK_* values for convenience.
struct WifiFilter {
  bool mgmt = true;
  bool ctrl = false;
  bool data = true;
  bool misc = false;
};

using WifiRxCallback = void (*)(void* ctx,
                                const uint8_t* frame, size_t len,
                                const WifiRxMeta& meta);

class IWifiMonitor {
public:
  virtual ~IWifiMonitor() = default;

  virtual bool start(const WifiFilter& filter, uint8_t channel) = 0;
  virtual void stop() = 0;
  virtual bool set_channel(uint8_t channel) = 0;
  virtual void set_callback(WifiRxCallback cb, void* ctx) = 0;

  virtual bool running() const = 0;
  virtual uint8_t channel() const = 0;
};

}  // namespace yui

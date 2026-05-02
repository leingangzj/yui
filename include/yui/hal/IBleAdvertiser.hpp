#pragma once
// IBleAdvertiser — BLE advertisement TX. Used by BleSpamApp to cycle
// through nuisance / fingerprintable advertisement payloads (e.g.
// Apple's continuity proximity broadcasts, Samsung Easy Setup pings).
//
// Yui ships this with Esp32BleAdvertiser as a STUB — the real impl
// requires NimBLE's GAP advertise APIs and is gated behind v0.3
// hardware bring-up (BLE central + advertiser concurrent with WiFi
// monitor mode is delicate timing).
#include <cstdint>
#include <cstddef>

namespace yui {

class IBleAdvertiser {
public:
  virtual ~IBleAdvertiser() = default;

  // Replace the current advertisement payload. payload is the raw 31-byte
  // BLE adv data (pre-formatted; caller is responsible for the structure).
  virtual bool set_payload(const uint8_t* payload, size_t len) = 0;

  virtual bool enable() = 0;     // start advertising at default cadence
  virtual void disable() = 0;
  virtual bool active() const = 0;
};

}  // namespace yui

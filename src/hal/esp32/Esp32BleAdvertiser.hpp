#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)
// Esp32BleAdvertiser — STUB. Real impl uses NimBLE's GAP advertise
// APIs (esp_ble_gap_set_raw_adv_data + start). Gated to v0.3
// hardware bring-up since BLE TX concurrent with WiFi monitor mode
// is timing-sensitive.
#include "yui/hal/IBleAdvertiser.hpp"

namespace yui {

class Esp32BleAdvertiser : public IBleAdvertiser {
public:
  bool set_payload(const uint8_t* /*p*/, size_t /*len*/) override { return false; }
  bool enable()  override { return false; }
  void disable() override { active_ = false; }
  bool active() const override { return active_; }

private:
  bool active_ = false;
};

}  // namespace yui
#endif

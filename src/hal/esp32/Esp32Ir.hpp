#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IIr.hpp"
#include <Arduino.h>

namespace yui {

// v0.0 placeholder. The Arduino-ESP32 release shipped with this PIO
// platform-espressif32 doesn't expose the new RMT TX API (driver/rmt_tx.h);
// we'll wire either IRremoteESP8266 or the legacy `driver/rmt.h` once we
// flash to real hardware and can confirm the IR LED GPIO + carrier path.
//
// For now: count "sends" so the app's UI shows "blast!" feedback, but
// nothing actually radiates. App logic is fully exercised via FakeIr in
// native tests.
class Esp32Ir : public IIr {
public:
  bool init() override { return true; }

  bool send_nec(uint16_t /*addr*/, uint16_t /*cmd*/, uint16_t /*carrier*/) override {
    ++count_;
    return true;
  }

  uint32_t sent_count() const override { return count_; }

private:
  uint32_t count_ = 0;
};

}  // namespace yui
#endif

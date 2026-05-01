#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IIr.hpp"
#include "yui/config/pins.hpp"
#include <IRsend.h>

namespace yui {

// NEC IR transmit on the ADV's IR LED (GPIO 44). Backed by IRremoteESP8266.
class Esp32Ir : public IIr {
public:
  Esp32Ir() : ir_(pins::kIrTx) {}

  bool init() override {
    if (inited_) return true;
    ir_.begin();
    inited_ = true;
    return true;
  }

  bool send_nec(uint16_t addr, uint16_t cmd, uint16_t /*carrier_khz*/) override {
    if (!init()) return false;
    // IRsend::sendNEC takes the full 32-bit code: addr16 | cmd16. We let the
    // caller pre-build whichever NEC variant they need; here we follow the
    // common "addr (16) << 16 | cmd (16)" convention.
    const uint64_t data = (static_cast<uint64_t>(addr) << 16) | cmd;
    ir_.sendNEC(data, 32);
    ++count_;
    return true;
  }

  uint32_t sent_count() const override { return count_; }

private:
  IRsend   ir_;
  bool     inited_ = false;
  uint32_t count_  = 0;
};

}  // namespace yui
#endif

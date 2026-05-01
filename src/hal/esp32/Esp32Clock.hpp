#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IClock.hpp"
#include <Arduino.h>
#include <time.h>

namespace yui {

class Esp32Clock : public IClock {
public:
  uint32_t millis() const override { return ::millis(); }
  void delay_ms(uint32_t ms) override { ::delay(ms); }
  uint64_t epoch_seconds() const override {
    time_t now = ::time(nullptr);
    // SNTP unset → time() returns ~0 (unix epoch 1970). Treat anything
    // before 2020-01-01 as "not synced" so callers can render --:--:--.
    if (now < 1577836800LL) return 0;
    return static_cast<uint64_t>(now);
  }
};

}  // namespace yui
#endif

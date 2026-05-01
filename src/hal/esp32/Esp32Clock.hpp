#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IClock.hpp"
#include <Arduino.h>

namespace yui {

class Esp32Clock : public IClock {
public:
  uint32_t millis() const override { return ::millis(); }
  void delay_ms(uint32_t ms) override { ::delay(ms); }
};

}  // namespace yui
#endif

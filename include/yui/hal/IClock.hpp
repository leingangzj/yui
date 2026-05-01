#pragma once
#include <cstdint>

namespace yui {

class IClock {
public:
  virtual ~IClock() = default;
  virtual uint32_t millis() const = 0;
  virtual void delay_ms(uint32_t ms) = 0;
  // Real-time clock in Unix epoch seconds. Returns 0 until a sync
  // (NTP, RTC, manual) has populated the system clock.
  virtual uint64_t epoch_seconds() const = 0;
};

}  // namespace yui

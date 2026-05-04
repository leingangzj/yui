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

  // Microsecond monotonic clock. Default impl converts millis() (so
  // existing backends keep compiling) — backends override for better
  // precision. Used by RollJam, edge-toggle TX, anything timing-tight.
  virtual uint64_t micros() const { return static_cast<uint64_t>(millis()) * 1000ULL; }

  // Busy-wait us. Default delegates to delay_ms (rounded up) — backends
  // override with esp_rom_delay_us / equivalent for sub-ms accuracy.
  virtual void delay_us(uint32_t us) {
    if (us < 1000) {
      // Fall through to a tiny ms delay — accurate enough for the
      // native test path (which doesn't care about real time).
      if (us > 0) delay_ms(1);
    } else {
      delay_ms((us + 999) / 1000);
    }
  }
};

}  // namespace yui

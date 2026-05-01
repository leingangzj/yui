#pragma once
#include <cstdint>

namespace yui {

class IClock {
public:
  virtual ~IClock() = default;
  virtual uint32_t millis() const = 0;
  virtual void delay_ms(uint32_t ms) = 0;
};

}  // namespace yui

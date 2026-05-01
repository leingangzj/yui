#pragma once
#include "yui/hal/IClock.hpp"

namespace yui {

class FakeClock : public IClock {
public:
  uint32_t millis() const override { return now_; }
  void delay_ms(uint32_t ms) override { now_ += ms; }
  void advance(uint32_t ms) { now_ += ms; }
  void set(uint32_t ms) { now_ = ms; }

private:
  mutable uint32_t now_ = 0;
};

}  // namespace yui

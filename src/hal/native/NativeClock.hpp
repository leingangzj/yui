#pragma once
#include "yui/hal/IClock.hpp"

namespace yui {

class FakeClock : public IClock {
public:
  uint32_t millis() const override { return now_; }
  void delay_ms(uint32_t ms) override { now_ += ms; }
  uint64_t epoch_seconds() const override { return epoch_; }
  void advance(uint32_t ms) { now_ += ms; }
  void set(uint32_t ms) { now_ = ms; }
  void set_epoch(uint64_t s) { epoch_ = s; }

private:
  mutable uint32_t now_   = 0;
  uint64_t         epoch_ = 0;
};

}  // namespace yui

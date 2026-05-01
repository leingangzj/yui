#pragma once
#include "yui/hal/IIr.hpp"

namespace yui {

class FakeIr : public IIr {
public:
  bool init() override { initialized_ = true; return true; }
  bool send_nec(uint16_t addr, uint16_t cmd, uint16_t /*carrier*/) override {
    last_addr_ = addr;
    last_cmd_  = cmd;
    ++count_;
    return true;
  }
  uint32_t sent_count() const override { return count_; }
  uint16_t last_addr() const { return last_addr_; }
  uint16_t last_cmd()  const { return last_cmd_; }

private:
  bool     initialized_ = false;
  uint32_t count_       = 0;
  uint16_t last_addr_   = 0;
  uint16_t last_cmd_    = 0;
};

}  // namespace yui

#pragma once
#include "yui/hal/IBleAdvertiser.hpp"
#include <cstring>
#include <vector>

namespace yui {

class FakeBleAdvertiser : public IBleAdvertiser {
public:
  bool set_payload(const uint8_t* payload, size_t len) override {
    if (!payload && len) return false;
    last_payload_.assign(payload, payload + len);
    ++set_count_;
    return true;
  }
  bool enable()  override { active_ = true; ++enable_count_; return true; }
  void disable() override { active_ = false; }
  bool active() const override { return active_; }

  // Test hooks
  int    set_count()    const { return set_count_; }
  int    enable_count() const { return enable_count_; }
  const std::vector<uint8_t>& last_payload() const { return last_payload_; }

private:
  std::vector<uint8_t> last_payload_;
  bool active_ = false;
  int  set_count_    = 0;
  int  enable_count_ = 0;
};

}  // namespace yui

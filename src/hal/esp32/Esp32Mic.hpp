#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IMic.hpp"
#include <M5Unified.h>
#include <cstring>

namespace yui {

class Esp32Mic : public IMic {
public:
  bool init() override {
    if (inited_) return true;
    inited_ = M5.Mic.begin();
    return inited_;
  }

  size_t read_samples(int16_t* out, size_t cap) override {
    if (!init()) return 0;
    return M5.Mic.record(out, cap, kSampleRate, false /*stereo*/) ? cap : 0;
  }

private:
  static constexpr uint32_t kSampleRate = 16000;
  bool inited_ = false;
};

}  // namespace yui
#endif

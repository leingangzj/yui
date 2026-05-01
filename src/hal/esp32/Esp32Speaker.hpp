#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/ISpeaker.hpp"
#include <M5Unified.h>

namespace yui {

class Esp32Speaker : public ISpeaker {
public:
  bool init() override {
    if (inited_) return true;
    inited_ = M5.Speaker.begin();
    if (inited_) M5.Speaker.setVolume(96);
    return inited_;
  }
  void tone(uint32_t freq_hz, uint32_t duration_ms) override {
    if (!init()) return;
    M5.Speaker.tone(static_cast<float>(freq_hz),
                    static_cast<uint32_t>(duration_ms));
  }
  void stop() override {
    if (inited_) M5.Speaker.stop();
  }

private:
  bool inited_ = false;
};

}  // namespace yui
#endif

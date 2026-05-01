#pragma once
#include <cstdint>

namespace yui {

class ISpeaker {
public:
  virtual ~ISpeaker() = default;
  virtual bool init() = 0;
  // Plays a sine tone at `freq_hz` for `duration_ms` (non-blocking on
  // hardware; on tests it just records the call).
  virtual void tone(uint32_t freq_hz, uint32_t duration_ms) = 0;
  virtual void stop() = 0;
};

}  // namespace yui

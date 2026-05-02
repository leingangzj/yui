#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IKeyboard.hpp"
#include "yui/hal/IRemote.hpp"
#include "yui/drivers/Tca8418.hpp"
#include "yui/drivers/AdvKeymap.hpp"
#include <M5Unified.h>

namespace yui {

// Cardputer-ADV keyboard backend.
// TCA8418 on M5.In_I2C @ 0x34 — configured as 7×8 matrix and remapped into
// the logical 4×14 ADV key grid by AdvKeymap. Tracks modifier state across
// press/release events so Shift/Fn produce the correct char.
class Esp32Keyboard : public IKeyboard {
public:
  Esp32Keyboard() : tca_(M5.In_I2C) {}

  // True once the TCA8418 acknowledged its config registers. Useful for
  // boot-time diagnostics — the Shell can flag a missing keypad before the
  // user starts wondering why nothing types.
  bool initialized() const { return initialized_; }

  // Optional: when wired, the remote viewer's key inbox is drained before
  // each I²C poll, so browser keystrokes are first-class.
  void set_remote(IRemote* r) { remote_ = r; }

  bool poll(KeyEvent& out) override {
    if (remote_ && remote_->running() && remote_->poll_key(out)) return true;

    if (!initialized_) {
      // Throttle init retries. The chip either answers within a few hundred
      // ms of I²C begin() or it isn't on the bus — hammering writeRegister8
      // every 33 ms wedges the bus and floods the serial log.
      const uint32_t now = millis();
      if (now - last_init_attempt_ms_ < kInitRetryMs) return false;
      last_init_attempt_ms_ = now;

      initialized_ = tca_.init_matrix(7, 8);
      if (!initialized_) {
        if (++init_failures_ == 1 || init_failures_ % 30 == 0) {
          Serial.printf("[kbd] TCA8418 init failed (attempt %u)\n",
                        init_failures_);
        }
        return false;
      }
      Serial.println("[kbd] TCA8418 ready");
    }

    uint8_t code = 0;
    bool    pressed = false;
    if (!tca_.poll(code, pressed)) return false;

    // Update modifier state if this was a modifier key, then emit an event.
    const auto pos = adv_decode_pos(code);
    if (pos.valid) {
      const auto& cell = adv_cell(pos.row, pos.col);
      if (cell.key == Key::Shift) shift_ = pressed;
      if (cell.key == Key::Fn)    fn_    = pressed;
      if (cell.key == Key::Ctrl)  ctrl_  = pressed;
      if (cell.key == Key::Alt)   alt_   = pressed;
    }

    return adv_make_event(code, pressed, shift_, fn_, ctrl_, alt_, out);
  }

private:
  static constexpr uint32_t kInitRetryMs = 250;

  Tca8418  tca_;
  IRemote* remote_ = nullptr;
  bool     initialized_ = false;
  uint32_t last_init_attempt_ms_ = 0;
  uint32_t init_failures_ = 0;
  bool     shift_ = false, fn_ = false, ctrl_ = false, alt_ = false;
};

}  // namespace yui
#endif

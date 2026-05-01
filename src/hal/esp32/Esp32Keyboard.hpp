#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IKeyboard.hpp"
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

  bool poll(KeyEvent& out) override {
    if (!initialized_) {
      initialized_ = tca_.init_matrix(7, 8);
      if (!initialized_) return false;
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
  Tca8418 tca_;
  bool    initialized_ = false;
  bool    shift_ = false, fn_ = false, ctrl_ = false, alt_ = false;
};

}  // namespace yui
#endif

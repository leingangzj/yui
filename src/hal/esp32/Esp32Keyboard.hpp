#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IKeyboard.hpp"
#include "yui/drivers/Tca8418.hpp"
#include <M5Unified.h>

namespace yui {

// Cardputer-ADV keyboard backend.
//
// Hardware: TI TCA8418 I²C keypad controller on M5.In_I2C, addr 0x34.
//
// v0.0 status: this driver reads real key events but the keymap from
// TCA8418 raw keycodes (1–80) → Yui Key / printable char is intentionally
// a placeholder until the ADV schematic is confirmed against actual
// hardware. For now every detected press emits Key::Enter, which is enough
// to drive the Splash → Launcher transition and exercise app entry.
//
// TODO(v0.0.2): replace `map_keycode()` with the real ADV layout once we
// can observe live keycode values on the device.
class Esp32Keyboard : public IKeyboard {
public:
  Esp32Keyboard() : tca_(M5.In_I2C) {}

  bool poll(KeyEvent& out) override {
    if (!initialized_) {
      initialized_ = tca_.init();
      if (!initialized_) return false;
    }

    uint8_t code = 0;
    bool    pressed = false;
    if (!tca_.poll(code, pressed)) return false;

    out      = KeyEvent{};
    out.key  = map_keycode(code);
    out.down = pressed;
    return true;
  }

private:
  static Key map_keycode(uint8_t /*code*/) {
    // Placeholder: any key acts as Enter. See TODO above.
    return Key::Enter;
  }

  Tca8418 tca_;
  bool    initialized_ = false;
};

}  // namespace yui
#endif

#pragma once
// Minimal driver for TI TCA8418 I²C keypad controller.
// Datasheet: https://www.ti.com/product/TCA8418
//
// Driven via M5Unified's I²C wrapper so we share the bus with the rest of
// the M5 stack (display init, IMU, audio codec). The Cardputer ADV places
// the TCA8418 at addr 0x34 on the *internal* I²C bus (M5.In_I2C).

#if defined(YUI_TARGET_CARDPUTER_ADV)

#include <M5Unified.h>
#include <cstdint>

namespace yui {

class Tca8418 {
public:
  static constexpr uint8_t kI2cAddr = 0x34;
  static constexpr uint32_t kI2cFreq = 100000;

  // Register map (subset).
  static constexpr uint8_t REG_CFG          = 0x01;
  static constexpr uint8_t REG_INT_STAT     = 0x02;
  static constexpr uint8_t REG_KEY_LCK_EC   = 0x03;
  static constexpr uint8_t REG_KEY_EVENT_A  = 0x04;
  static constexpr uint8_t REG_KP_GPIO_1    = 0x1D;  // R0..R7
  static constexpr uint8_t REG_KP_GPIO_2    = 0x1E;  // R8..R9 + C0..C5
  static constexpr uint8_t REG_KP_GPIO_3    = 0x1F;  // C6..C9

  explicit Tca8418(m5::I2C_Class& i2c) : i2c_(i2c) {}

  bool init() {
    if (!i2c_.writeRegister8(kI2cAddr, REG_KP_GPIO_1, 0xFF, kI2cFreq)) return false;
    if (!i2c_.writeRegister8(kI2cAddr, REG_KP_GPIO_2, 0xFF, kI2cFreq)) return false;
    if (!i2c_.writeRegister8(kI2cAddr, REG_KP_GPIO_3, 0xFF, kI2cFreq)) return false;
    if (!i2c_.writeRegister8(kI2cAddr, REG_CFG,       0x01, kI2cFreq)) return false;
    return true;
  }

  // Drain one event from the FIFO. Returns false if the FIFO is empty or
  // I²C has no responder. (We can't fully distinguish "no event" from "no
  // chip" with the M5 API's short-form reads — but the FIFO-empty check
  // makes a missing chip mostly harmless: it just always returns false.)
  bool poll(uint8_t& out_keycode, bool& out_is_press) {
    const uint8_t lck = i2c_.readRegister8(kI2cAddr, REG_KEY_LCK_EC, kI2cFreq);
    if ((lck & 0x0F) == 0) return false;

    const uint8_t ev = i2c_.readRegister8(kI2cAddr, REG_KEY_EVENT_A, kI2cFreq);
    out_is_press = (ev & 0x80) != 0;
    out_keycode  = ev & 0x7F;
    return out_keycode != 0;
  }

private:
  m5::I2C_Class& i2c_;
};

}  // namespace yui
#endif

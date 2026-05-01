#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IKeyboard.hpp"

namespace yui {

// v0.0 stub — produces no events.
//
// The Cardputer ADV uses a TCA8418 I²C keypad controller (NOT GPIO matrix
// scanning like the base Cardputer). Real impl in v0.1 will:
//   - init the TCA8418 on the internal I²C bus
//   - poll its key event FIFO each tick
//   - map (row, col) to Yui Key enum + ASCII char
class Esp32Keyboard : public IKeyboard {
public:
  bool poll(KeyEvent& /*out*/) override { return false; }
};

}  // namespace yui
#endif

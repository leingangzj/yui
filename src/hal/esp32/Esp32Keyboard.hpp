#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IKeyboard.hpp"
#include <M5Unified.h>
#include <M5Cardputer.h>

namespace yui {

// TODO(v0.1): map M5Cardputer.Keyboard.keysState() → Yui KeyEvents.
// For now this is a stub so v0.0 compiles.
class Esp32Keyboard : public IKeyboard {
public:
  bool poll(KeyEvent& /*out*/) override { return false; }
};

}  // namespace yui
#endif

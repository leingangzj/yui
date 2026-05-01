#pragma once
#include "yui/hal/IDisplay.hpp"
#include "yui/hal/IKeyboard.hpp"
#include "yui/hal/IClock.hpp"
#include "yui/hal/ILog.hpp"

namespace yui {

struct Hal {
  IDisplay&  display;
  IKeyboard& keyboard;
  IClock&    clock;
  ILog&      log;
};

}  // namespace yui

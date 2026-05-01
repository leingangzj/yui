#pragma once
#include "yui/hal/Hal.hpp"
#include "yui/hal/IKeyboard.hpp"

namespace yui {

// Base for every Yui app. Apps don't draw or pump events themselves — the
// Shell tells them when to render, ticks them at a steady cadence, and feeds
// them key events. All HAL access flows through the Hal& given on entry.
class App {
public:
  virtual ~App() = default;

  virtual const char* name() const = 0;

  virtual void on_enter(Hal& /*hal*/) {}
  virtual void on_key(KeyEvent /*k*/) {}
  virtual void tick(uint32_t /*now_ms*/) {}
  virtual void render(IDisplay& d) = 0;
  virtual void on_exit() {}
};

}  // namespace yui

#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IDisplay.hpp"
#include <M5Unified.h>

namespace yui {

class Esp32Display : public IDisplay {
public:
  int width() const override  { return M5.Display.width(); }
  int height() const override { return M5.Display.height(); }
  void clear(Color c) override { M5.Display.fillScreen(c); }
  void put_pixel(int x, int y, Color c) override { M5.Display.drawPixel(x, y, c); }
  void fill_rect(Rect r, Color c) override { M5.Display.fillRect(r.x, r.y, r.w, r.h, c); }
  void draw_text(int x, int y, const char* s, Color fg, Color bg) override {
    M5.Display.setTextColor(fg, bg);
    M5.Display.setCursor(x, y);
    M5.Display.print(s);
  }
  void flush() override { /* M5GFX is immediate-mode here */ }
};

}  // namespace yui
#endif

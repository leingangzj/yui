#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IDisplay.hpp"
#include <M5Unified.h>

namespace yui {

// Offscreen-buffered display backend.
//
// All draw calls go into an M5Canvas sized to the panel; flush() pushes the
// whole canvas to M5.Display in one DMA blit. This eliminates the diagonal
// tearing caused by repainting a live framebuffer at 30 FPS.
//
// Memory: 240×135×2 = 64,800 bytes in 16-bit color. Fits in S3 internal SRAM
// even with PSRAM disabled.
class Esp32Display : public IDisplay {
public:
  bool begin() {
    canvas_.setColorDepth(16);
    if (!canvas_.createSprite(M5.Display.width(), M5.Display.height())) {
      canvas_init_failed_ = true;
      return false;
    }
    canvas_.setTextSize(1);
    canvas_.fillSprite(TFT_BLACK);
    canvas_.pushSprite(0, 0);
    return true;
  }

  int width() const override {
    return canvas_init_failed_ ? M5.Display.width() : canvas_.width();
  }
  int height() const override {
    return canvas_init_failed_ ? M5.Display.height() : canvas_.height();
  }

  void clear(Color c) override { target_().fillScreen(c); }
  void put_pixel(int x, int y, Color c) override { target_().drawPixel(x, y, c); }
  void fill_rect(Rect r, Color c) override { target_().fillRect(r.x, r.y, r.w, r.h, c); }
  void draw_text(int x, int y, const char* s, Color fg, Color bg) override {
    auto& t = target_();
    t.setTextColor(fg, bg);
    t.setCursor(x, y);
    t.print(s);
  }
  void flush() override {
    if (canvas_init_failed_) return;
    canvas_.pushSprite(0, 0);
  }

private:
  // If sprite allocation fails (low heap), fall back to immediate mode so the
  // device at least boots — tearing returns but it's better than a black brick.
  // Both M5.Display and M5Canvas derive from lgfx::LGFXBase, so we hand back
  // the common base when picking the active draw target.
  lgfx::LGFXBase& target_() {
    if (canvas_init_failed_) return M5.Display;
    return canvas_;
  }

  M5Canvas canvas_{&M5.Display};
  bool canvas_init_failed_ = false;
};

}  // namespace yui
#endif

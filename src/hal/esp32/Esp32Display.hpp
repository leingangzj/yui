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
    canvas_.setFont(&fonts::Font0);
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

  // ── Text ─────────────────────────────────────────────────────────────
  // Single styled entry point. Every other text overload on IDisplay
  // delegates here.
  //
  // We pick a real bundled font per style:
  //   Title    → FreeSansBold12pt7b  (smooth, ~17 px)
  //   Body     → FreeSans9pt7b       (smooth, ~13 px)
  //   Caption  → Font0 size-1        (legacy 6×8, smallest)
  //   Mono     → Font0 size-1        (legacy 6×8 monospace)
  //
  // After drawing we restore Font0 size-1 so legacy callers that skipped
  // the style API still see the renderer in a known state.
  void draw_text_styled(int x, int y, const char* s,
                        Color fg, Color bg, FontStyle style) override {
    auto& t = target_();
    apply_font_(t, style);
    t.setTextColor(fg, bg);
    t.setCursor(x, y);
    t.print(s);
    reset_font_(t);
  }

  int text_width(const char* s, FontStyle style) override {
    auto& t = target_();
    apply_font_(t, style);
    const int w = static_cast<int>(t.textWidth(s));
    reset_font_(t);
    return w;
  }

  int line_height(FontStyle style) override {
    auto& t = target_();
    apply_font_(t, style);
    const int h = static_cast<int>(t.fontHeight());
    reset_font_(t);
    return h;
  }

  void draw_png(const uint8_t* data, std::size_t len, int x, int y) override {
    if (!data || len == 0) return;
    target_().drawPng(data, len, x, y);
  }

  void flush() override {
    if (canvas_init_failed_) return;
    canvas_.pushSprite(0, 0);
  }

  // Lets the remote viewer (Esp32WebRemote) capture the same backing
  // sprite we push to the panel. Returns nullptr if sprite alloc failed.
  M5Canvas* canvas() {
    return canvas_init_failed_ ? nullptr : &canvas_;
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

  // Map a FontStyle to a real M5GFX bundled font. Title and Body get
  // proportional smooth fonts (FreeSans family — ships with M5GFX);
  // Caption and Mono use the built-in Font0 6×8 grid for compactness.
  static void apply_font_(lgfx::LGFXBase& t, FontStyle style) {
    switch (style) {
      case FontStyle::Title:
        t.setFont(&fonts::FreeSansBold12pt7b);
        t.setTextSize(1);
        break;
      case FontStyle::Body:
        t.setFont(&fonts::FreeSans9pt7b);
        t.setTextSize(1);
        break;
      case FontStyle::Caption:
      case FontStyle::Mono:
      default:
        t.setFont(&fonts::Font0);
        t.setTextSize(1);
        break;
    }
  }

  static void reset_font_(lgfx::LGFXBase& t) {
    t.setFont(&fonts::Font0);
    t.setTextSize(1);
  }

  M5Canvas canvas_{&M5.Display};
  bool canvas_init_failed_ = false;
};

}  // namespace yui
#endif

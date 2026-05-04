#pragma once
#include "yui/types.hpp"
#include <cstddef>
#include <cstdint>

namespace yui {

// ── Font styles ─────────────────────────────────────────────────────────
// Every text-drawing path in Yui lands at draw_text_styled(); pick a
// style and the backend renders with the matching font. Native (test)
// backend keeps a fixed 6×8 cell so existing tests stay deterministic;
// device backend maps each style to a real M5GFX bundled font.
//
//   Title    — page headers, splash titles, big readable copy. (~17 px)
//   Body     — default reading size. Chrome rows, app content. (~13 px)
//   Caption  — small hints, footers, secondary stats. (~8 px / size-1)
//   Mono     — fixed-width values: hex dumps, frequencies. (~8 px)
//
// Apps that don't pick a style get Body. The whole firmware harmonizes
// by routing every text path through this enum.
enum class FontStyle : uint8_t { Title, Body, Caption, Mono };

class IDisplay {
public:
  virtual ~IDisplay() = default;
  virtual int width() const  = 0;
  virtual int height() const = 0;
  virtual void clear(Color c) = 0;
  virtual void put_pixel(int x, int y, Color c) = 0;
  virtual void fill_rect(Rect r, Color c) = 0;

  // ── Text — single virtual entry point ───────────────────────────────
  // Every other draw_text* method delegates here so backends only have
  // to implement one path.
  virtual void draw_text_styled(int x, int y, const char* s,
                                Color fg, Color bg, FontStyle style) = 0;

  // Pixel width of `s` rendered at the given style. Backends ask their
  // font engine; native impl returns strlen × 6.
  virtual int text_width(const char* s, FontStyle style) = 0;

  // Line height (cap-to-descender) for the given style. Used by chrome
  // to lay rows out without hardcoding pixel sizes per font.
  virtual int line_height(FontStyle style) = 0;

  // ── Convenience overloads ───────────────────────────────────────────
  // No style → Body.
  void draw_text(int x, int y, const char* s, Color fg, Color bg) {
    draw_text_styled(x, y, s, fg, bg, FontStyle::Body);
  }

  // Legacy scaled text — kept so the existing 51 source files don't have
  // to change in this commit. Mapping:
  //   scale ≤1 → Caption    (matches old size-1 6×8)
  //   scale  2 → Body       (matches old size-2 chunky)
  //   scale ≥3 → Title      (proportional bold)
  // New code should call draw_text_styled directly with a FontStyle.
  void draw_text_scaled(int x, int y, const char* s,
                        Color fg, Color bg, int scale) {
    FontStyle st;
    if (scale <= 1)       st = FontStyle::Caption;
    else if (scale == 2)  st = FontStyle::Body;
    else                  st = FontStyle::Title;
    draw_text_styled(x, y, s, fg, bg, st);
  }

  // ── Misc ────────────────────────────────────────────────────────────
  // Decode + blit a PNG. Optional — backends without a PNG decoder
  // ignore the call.
  virtual void draw_png(const uint8_t* /*data*/, std::size_t /*len*/,
                        int /*x*/, int /*y*/) {}

  virtual void flush() = 0;
};

}  // namespace yui

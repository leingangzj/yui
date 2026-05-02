#pragma once
// Chrome — the shared system-look every Yui app paints with.
//
// Use these helpers instead of hand-coding rectangles + text everywhere.
// One call per band, consistent geometry, single source of truth.
//
// Typical app render():
//
//   void render(IDisplay& d) override {
//     d.clear(ui::kSurface);
//     ui::Chrome::header(d, "My App", subtitle ? subtitle : nullptr);
//     // ... body, drawn against ui::kBodyTopY + n * ui::kBodyLineH ...
//     ui::Chrome::footer(d, "Enter:open  Esc:back");
//     d.flush();
//   }
//
// See docs/UI_CONVENTIONS.md for the full pattern.

#include "yui/hal/IDisplay.hpp"
#include "yui/types.hpp"
#include "yui/ui/Tokens.hpp"
#include "yui/assets/sprite_frame.hpp"

#include <cstdint>

namespace yui::ui {

class Chrome {
public:
  // Top-of-screen accent strip. Title is rendered at 2× so it actually
  // owns the bar instead of looking apologetic. Optional right-aligned
  // subtitle (mode, status, count) draws at default size.
  static void header(IDisplay& d, const char* title,
                     const char* subtitle = nullptr) {
    d.fill_rect({0, 0, d.width(), kHeaderH}, kAccent);
    d.draw_text_scaled(kBodyPadX, 2, title, kOnAccent, kAccent, 2);
    if (subtitle && *subtitle) {
      const int sw = text_pixel_width(subtitle);
      d.draw_text(d.width() - sw - 4, 6, subtitle, kOnAccent, kAccent);
    }
  }

  // Bottom hint line. Always dark-red on white so it reads as a hint,
  // not a heading. Pass nullptr to skip.
  static void footer(IDisplay& d, const char* hint) {
    if (!hint || !*hint) return;
    d.draw_text(kBodyPadX, kFooterY, hint, kHint, kSurface);
  }

  // Highlighted list-row bar at index `row` (0-based). Used by any app
  // that has a vertical list of selectable items — settings, files,
  // todo, IR presets, etc. Matches the launcher's sub-view styling.
  static void list_row(IDisplay& d, int row, const char* label,
                       bool selected) {
    const int y_top  = kBodyTopY + row * kListRowH;
    const Color bg   = selected ? kAccent : kSurface;
    const Color fg   = selected ? kOnAccent : kAccent;
    if (selected) d.fill_rect({0, y_top - 2, d.width(), kListRowH}, bg);
    d.draw_text(kBodyPadX, y_top + 4, label, fg, bg);
  }

  // Small inline pill at (x,y). Useful for toggles, "REC" indicators,
  // mode badges. Returns the pixel width consumed.
  static int badge(IDisplay& d, int x, int y, const char* text, bool on) {
    const int w = text_pixel_width(text) + 6;
    const Color bg = on ? kAccent : kSurface;
    const Color fg = on ? kOnAccent : kAccentDark;
    if (on) d.fill_rect({x, y - 2, w, kBodyLineH}, bg);
    d.draw_text(x + 3, y, text, fg, bg);
    return w;
  }

  // Centered empty-state message in the body area. Optionally tiles a
  // small mascot sprite next to the message. Pass nullptr for sprite to
  // skip the koi (e.g. tests or non-thematic empty states).
  static void empty(IDisplay& d, const char* message,
                    const assets::SpriteFrame* mascot = nullptr,
                    int mascot_w = 0, int mascot_h = 0) {
    const int msg_w = text_pixel_width(message);
    const int total_w = (mascot ? mascot_w + 8 : 0) + msg_w;
    const int x0 = (d.width() - total_w) / 2;
    const int y0 = (kBodyTopY + kFooterY) / 2 - kCharH / 2;

    int x = x0;
    if (mascot) {
      d.draw_png(mascot->data, mascot->len,
                 x, y0 - (mascot_h - kCharH) / 2);
      x += mascot_w + 8;
    }
    d.draw_text(x, y0, message, kHint, kSurface);
  }

  // Dim line of value text rendered against the body grid. Helper for
  // the most common app body shape: stat rows.
  static void stat(IDisplay& d, int row, const char* label,
                   const char* value) {
    const int y = kBodyTopY + row * kBodyLineH;
    d.draw_text(kBodyPadX, y, label, kHint, kSurface);
    d.draw_text(kBodyPadX + 96, y, value, kOnSurface, kSurface);
  }
};

}  // namespace yui::ui

#pragma once
// UI design tokens. Every app's chrome — header / body grid / footer —
// is laid out against these constants, so a single tweak here moves the
// whole system together. See docs/UI_CONVENTIONS.md for the rationale.

#include "yui/types.hpp"

namespace yui::ui {

// Vertical bands.
inline constexpr int kHeaderH   = 20;          // tall enough for size-2 title
inline constexpr int kBodyTopY  = kHeaderH + 4; // first body baseline = 24
inline constexpr int kBodyLineH = 14;          // 7 lines fit
inline constexpr int kFooterY   = kScreenHeight - 12;

// Horizontal padding.
inline constexpr int kBodyPadX  = 8;

// Selection-row height (matches Launcher's app sub-view).
inline constexpr int kListRowH  = 16;

// Colour aliases that read better in chrome code.
inline constexpr Color kAccent     = kJapanRed;
inline constexpr Color kAccentDark = kJapanRedDark;
inline constexpr Color kSurface    = kWhite;
inline constexpr Color kOnSurface  = kBlack;
inline constexpr Color kOnAccent   = kWhite;
inline constexpr Color kHint       = kJapanRedDark;
// Warn: errors, failed states, and "this is firing right now" indicators.
// Visually louder than the accent so it pulls the eye to "stop and look."
// Same shade as the splash shimmer; shares one token because both roles
// reduce to "attention required."
inline constexpr Color kWarn       = kJapanRedBright;

// Default monospace glyph size for size-1 text (M5GFX 6×8 + 1 px gap).
inline constexpr int kCharW = 6;
inline constexpr int kCharH = 8;

inline constexpr int text_pixel_width(const char* s) {
  int n = 0;
  while (s && *s++) ++n;
  return n * kCharW;
}

}  // namespace yui::ui

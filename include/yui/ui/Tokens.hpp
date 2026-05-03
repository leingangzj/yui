#pragma once
// UI design tokens. Every app's chrome — header / body grid / footer —
// is laid out against these constants, so a single tweak here moves the
// whole system together. See docs/UI_CONVENTIONS.md for the rationale.
//
// Colour tokens are NOT constexpr — they're mutable inline globals so the
// runtime palette switcher (ThemeApp) can rewrite them. Apps read them
// per-frame, so the next render after apply_palette() picks up new values.

#include "yui/types.hpp"

namespace yui::ui {

// ── Geometry — fixed at compile time ─────────────────────────────────────
inline constexpr int kHeaderH   = 20;          // tall enough for size-2 title
inline constexpr int kBodyTopY  = kHeaderH + 4; // first body baseline = 24
inline constexpr int kBodyLineH = 14;          // size-1 body — 7 lines fit
inline constexpr int kBodyLineH2 = 22;         // size-2 body — 4-5 lines fit
inline constexpr int kFooterY   = kScreenHeight - 12;
inline constexpr int kBodyPadX  = 8;
inline constexpr int kListRowH  = 22;          // size-2 list rows — readable
                                               // from arm's-length on the
                                               // Cardputer's tiny panel

// Default monospace glyph size for size-1 text (M5GFX 6×8 + 1 px gap).
inline constexpr int kCharW = 6;
inline constexpr int kCharH = 8;

inline constexpr int text_pixel_width(const char* s) {
  int n = 0;
  while (s && *s++) ++n;
  return n * kCharW;
}

// ── Palette — runtime-mutable, default = Kohaku ──────────────────────────
// Apps read these through `ui::kAccent` etc; ThemeApp rewrites them.
inline Color kAccent     = kJapanRed;
inline Color kAccentDark = kJapanRedDark;
inline Color kSurface    = kWhite;
inline Color kOnSurface  = kBlack;
inline Color kOnAccent   = kWhite;
inline Color kHint       = kJapanRedDark;  // role alias of kAccentDark
// Warn: errors, failed states, and "this is firing right now" indicators.
// Visually louder than the accent so it pulls the eye to "stop and look."
inline Color kWarn       = kJapanRedBright;

// ── Palette presets (koi-varieties) ──────────────────────────────────────
// Each palette is 7 RGB565 values matching the role tokens above. Adding
// a new palette = one constexpr struct here + one entry in kPalettes[].

struct Palette {
  const char* id;          // stable NVS key (lowercase, no spaces)
  const char* label;       // human-readable name shown in ThemeApp
  Color accent;
  Color accent_dark;
  Color surface;
  Color on_surface;
  Color on_accent;
  Color warn;
};

// Hinomaru / red-on-white koi — the default Yui look.
inline constexpr Palette kKohaku = {
    "kohaku",   "Kohaku (red)",
    /*accent*/      0xB805,   // #BC002D hinomaru red
    /*accent_dark*/ 0x7800,   // #780000
    /*surface*/     0xFFFF,   // #FFFFFF
    /*on_surface*/  0x0000,   // #000000
    /*on_accent*/   0xFFFF,
    /*warn*/        0xF9CB,   // #FF4060
};

// Gold koi: warm gold accents on cream, sunset highlights for warn.
inline constexpr Palette kOgon = {
    "ogon",     "Ogon (gold)",
    /*accent*/      0xC4E4,   // ~#C49B0E mustard gold
    /*accent_dark*/ 0x6260,   // ~#665100 dark gold
    /*surface*/     0xFFDC,   // ~#FFFCE6 cream
    /*on_surface*/  0x18C3,   // ~#1A1A1F near-black
    /*on_accent*/   0xFFDC,
    /*warn*/        0xFC60,   // ~#FF8C00 sunset orange
};

// Asagi koi: blue-grey scales with red belly. Cool-toned chrome,
// hinomaru red as the "warn" punch so the koi heritage stays visible.
inline constexpr Palette kAsagi = {
    "asagi",    "Asagi (blue)",
    /*accent*/      0x33CD,   // ~#3478A8 slate blue
    /*accent_dark*/ 0x10A4,   // ~#1B3A52 deep slate
    /*surface*/     0xFFFF,   // #FFFFFF
    /*on_surface*/  0x0000,
    /*on_accent*/   0xFFFF,
    /*warn*/        0xB805,   // hinomaru red — koi belly accent
};

inline constexpr Palette kPalettes[] = { kKohaku, kOgon, kAsagi };
inline constexpr std::size_t kPaletteCount =
    sizeof(kPalettes) / sizeof(kPalettes[0]);

// Apply a palette to the live tokens. Apps re-read kAccent/etc every
// render() so the change is visible on the next frame.
inline void apply_palette(const Palette& p) {
  kAccent     = p.accent;
  kAccentDark = p.accent_dark;
  kSurface    = p.surface;
  kOnSurface  = p.on_surface;
  kOnAccent   = p.on_accent;
  kHint       = p.accent_dark;
  kWarn       = p.warn;
}

inline const Palette* find_palette(const char* id) {
  if (!id) return nullptr;
  for (std::size_t i = 0; i < kPaletteCount; ++i) {
    const char* a = kPalettes[i].id;
    bool eq = true;
    for (std::size_t k = 0; ; ++k) {
      if (a[k] != id[k]) { eq = false; break; }
      if (a[k] == 0)     break;
    }
    if (eq) return &kPalettes[i];
  }
  return nullptr;
}

}  // namespace yui::ui

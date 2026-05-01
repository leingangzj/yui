#pragma once
// UTF-8 Unicode Braille (U+2800..U+28FF) → 2×4 dot bitmap decoder.
//
// Each Braille glyph encodes 8 dots:
//
//     dot 1 (bit 0)   dot 4 (bit 3)
//     dot 2 (bit 1)   dot 5 (bit 4)
//     dot 3 (bit 2)   dot 6 (bit 5)
//     dot 7 (bit 6)   dot 8 (bit 7)
//
// The codepoint = U+2800 + dot_byte. Each glyph paints a 2-wide × 4-tall block.

#include <cstdint>
#include "yui/types.hpp"
#include "yui/hal/IDisplay.hpp"

namespace yui {

constexpr int kBrailleDx[8] = {0, 0, 0, 1, 1, 1, 0, 1};
constexpr int kBrailleDy[8] = {0, 1, 2, 0, 1, 2, 3, 3};

// Decode one UTF-8 Braille codepoint at *p. Advances p past the glyph.
// Returns 0 (no dots) for non-Braille bytes; advances p by 1 in that case.
inline uint8_t braille_decode(const char*& p) {
  unsigned char b1 = static_cast<unsigned char>(p[0]);
  if (b1 != 0xE2) {
    ++p;
    return 0;
  }
  unsigned char b2 = static_cast<unsigned char>(p[1]);
  unsigned char b3 = static_cast<unsigned char>(p[2]);
  if ((b2 & 0xFC) != 0xA0 || (b3 & 0xC0) != 0x80) {
    ++p;
    return 0;
  }
  p += 3;
  return static_cast<uint8_t>(((b2 & 0x03) << 6) | (b3 & 0x3F));
}

// Render UTF-8 Braille art onto a display.
// art   — null-terminated UTF-8 string with '\n' between rows
// x0,y0 — top-left origin in display pixels
// scale — pixels per dot (1, 2, 3, …)
// color_for(dx, dy) → Color : called per lit dot, dx/dy are dot coords
template <typename ColorFn>
void render_braille(IDisplay& d, const char* art, int x0, int y0, int scale,
                    ColorFn color_for) {
  int row = 0;
  int col = 0;
  const char* p = art;
  while (*p) {
    if (*p == '\n') {
      ++row;
      col = 0;
      ++p;
      continue;
    }
    if (*p == '\r') {
      ++p;
      continue;
    }
    uint8_t dots = braille_decode(p);
    for (int b = 0; b < 8; ++b) {
      if (dots & (1 << b)) {
        int dx = col * 2 + kBrailleDx[b];
        int dy = row * 4 + kBrailleDy[b];
        Color c = color_for(dx, dy);
        d.fill_rect({x0 + dx * scale, y0 + dy * scale, scale, scale}, c);
      }
    }
    ++col;
  }
}

}  // namespace yui

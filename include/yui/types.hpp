#pragma once
#include <cstdint>

namespace yui {

constexpr int kScreenWidth  = 240;
constexpr int kScreenHeight = 135;

using Color = uint16_t;  // RGB565 on device; ditto on native framebuffer

constexpr Color kBlack = 0x0000;
constexpr Color kWhite = 0xFFFF;
constexpr Color kRed   = 0xF800;
constexpr Color kGreen = 0x07E0;
constexpr Color kBlue  = 0x001F;

// Hinomaru — Japanese flag red (#BC002D), plus a brighter highlight tone
// and a darker shadow tone for shimmer / wordmark accents.
constexpr Color kJapanRed       = 0xB805;
constexpr Color kJapanRedBright = 0xF9CB;  // ≈ #FF4060, sweep highlight
constexpr Color kJapanRedDark   = 0x7800;  // ≈ #780000, shadow / version text

struct Point { int x, y; };
struct Rect  { int x, y, w, h; };

}  // namespace yui

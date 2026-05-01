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

struct Point { int x, y; };
struct Rect  { int x, y, w, h; };

}  // namespace yui

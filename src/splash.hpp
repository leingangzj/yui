#pragma once
#include "yui/hal/Hal.hpp"

namespace yui {

// Renders a simple splash. HAL-only, so it's testable.
inline void render_splash(IDisplay& d, const char* version) {
  d.clear(kBlack);
  d.fill_rect({0, 0, d.width(), 24}, kBlue);
  d.draw_text(8, 4, "Yui", kWhite, kBlue);
  d.draw_text(8, 40, "M5Cardputer ADV", kWhite, kBlack);
  d.draw_text(8, 56, version, kGreen, kBlack);
  d.draw_text(8, d.height() - 16, "press any key", kWhite, kBlack);
  d.flush();
}

}  // namespace yui

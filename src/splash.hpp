#pragma once
// Yui splash screen — animated hinomaru-red koi on white, with a single-column
// highlight that sweeps left→right across the koi to evoke shimmer / motion.
//
// HAL-only. Same code path on device and in native unit tests.

#include "yui/hal/Hal.hpp"
#include "yui/gfx/Braille.hpp"
#include "yui/gfx/koi_art.hpp"
#include "yui/types.hpp"

namespace yui {

constexpr int   kSplashScale       = 2;     // pixels per Braille dot
constexpr uint32_t kShimmerStepMs  = 28;    // ms per highlight column step
constexpr int   kShimmerWidth      = 3;     // bright band width in dots

// Render one frame of the splash for the given elapsed time since splash start.
// Pure HAL — testable from the native env.
inline void render_splash(IDisplay& d, const char* version, uint32_t elapsed_ms) {
  d.clear(kWhite);

  const int art_w_px = kKoiArtDotsW * kSplashScale;
  const int art_h_px = kKoiArtDotsH * kSplashScale;
  const int x0 = (d.width()  - art_w_px) / 2;
  const int y0 = 6;

  // Highlight column wraps every (kKoiArtDotsW + kShimmerWidth) steps so the
  // band cleanly enters from the left and exits to the right.
  const int sweep_span = kKoiArtDotsW + kShimmerWidth;
  const int hl_lead = static_cast<int>((elapsed_ms / kShimmerStepMs) % sweep_span);

  auto color_for = [hl_lead](int dx, int /*dy*/) -> Color {
    int delta = hl_lead - dx;
    if (delta >= 0 && delta < kShimmerWidth) return kJapanRedBright;
    return kJapanRed;
  };

  render_braille(d, kKoiArt, x0, y0, kSplashScale, color_for);

  // Wordmark + version under the koi.
  const int text_y = y0 + art_h_px + 2;
  d.draw_text(d.width() / 2 - 9,  text_y,      "Yui",   kJapanRed,     kWhite);
  d.draw_text(d.width() / 2 - 24, text_y + 12, version, kJapanRedDark, kWhite);

  d.flush();
}

}  // namespace yui

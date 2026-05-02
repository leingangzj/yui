#pragma once
// Boot animation: 24-frame koi sequence + final-state hold. Plays through
// IDisplay::draw_png; on backends that don't decode PNG (the native test
// env) the calls are silently no-op'd, so the firmware boot path stays
// portable.
//
// Timing matches Zac's reference design:
//   • 173 ms per frame for frames 0..22
//   • 4173 ms hold on frame 23 (the "Yui" wordmark)
// Press a key after the minimum animation length to skip the hold.

#include "yui/hal/IDisplay.hpp"
#include "yui/types.hpp"
#include "yui/assets/boot_frames.hpp"
#include <cstdint>

namespace yui {

constexpr uint32_t kBootFrameMs       = 173;
constexpr uint32_t kBootFinalHoldMs   = 4173;

inline std::size_t boot_frame_index_at(uint32_t elapsed_ms) {
  const std::size_t n = assets::kBootFrameCount;
  if (n == 0) return 0;
  const std::size_t i = elapsed_ms / kBootFrameMs;
  return (i >= n) ? (n - 1) : i;
}

// Total minimum splash duration: every frame plays once, then the final
// frame holds for kBootFinalHoldMs.
inline constexpr uint32_t kBootAnimationTotalMs =
    static_cast<uint32_t>(assets::kBootFrameCount - 1) * kBootFrameMs +
    kBootFinalHoldMs;

inline void render_boot_animation(IDisplay& d, uint32_t elapsed_ms) {
  d.clear(kBlack);
  const std::size_t i = boot_frame_index_at(elapsed_ms);
  const auto& f = assets::kBootFrames[i];
  d.draw_png(f.data, f.len, 0, 0);
  d.flush();
}

}  // namespace yui

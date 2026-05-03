#pragma once
// Bubble-level toy. Reads gravity vector from the IMU, draws a circular
// crosshair with a moving "bubble" indicating tilt.
#include "yui/app/App.hpp"
#include "yui/hal/IImu.hpp"
#include "yui/types.hpp"
#include <cmath>
#include <cstdio>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class ImuApp : public App {
public:
  explicit ImuApp(IImu& imu) : imu_(imu) {}
  const char* name() const override { return "IMU Toys"; }
  Category    category() const override { return Category::Fun; }

  void on_enter(Hal& /*hal*/) override { imu_.init(); }

  void tick(uint32_t /*now_ms*/) override {
    AccelXYZ a{};
    if (imu_.read_accel(a)) last_ = a;
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "Bubble Level");

    // Crosshair circle area
    const int cx = d.width() / 2;
    const int cy = 16 + (d.height() - 16) / 2 - 4;
    const int r  = 36;

    // Crosshair lines
    d.fill_rect({cx - r, cy, 2 * r, 1}, kJapanRedDark);
    d.fill_rect({cx, cy - r, 1, 2 * r}, kJapanRedDark);

    // Outline: draw a square frame as a stand-in for a circle (M5GFX has
    // proper drawCircle but our HAL is intentionally minimal).
    for (int t = 0; t < 4; ++t) {
      d.fill_rect({cx - r,     cy - r + t, 2 * r, 1}, kJapanRedDark);
      d.fill_rect({cx - r,     cy + r - t, 2 * r, 1}, kJapanRedDark);
      d.fill_rect({cx - r + t, cy - r,     1, 2 * r}, kJapanRedDark);
      d.fill_rect({cx + r - t, cy - r,     1, 2 * r}, kJapanRedDark);
    }

    // Bubble: -X tilt → bubble drifts +x (level rule). Map a.x ∈ [-1,1] → ±r.
    const float ax = clamp01_(last_.x);
    const float ay = clamp01_(last_.y);
    const int bx = cx + static_cast<int>(-ax * (r - 4));
    const int by = cy + static_cast<int>( ay * (r - 4));
    d.fill_rect({bx - 3, by - 3, 7, 7}, kJapanRed);

    char line[32];
    std::snprintf(line, sizeof(line), "x=%+.2f y=%+.2f", last_.x, last_.y);
    d.draw_text(8, d.height() - 14, line, kJapanRedDark, kWhite);
    d.flush();
  }

  AccelXYZ last_accel() const { return last_; }

private:
  static float clamp01_(float v) {
    if (v < -1.f) return -1.f;
    if (v >  1.f) return  1.f;
    return v;
  }

  IImu&    imu_;
  AccelXYZ last_{0.f, 0.f, 1.f};
};

}  // namespace yui

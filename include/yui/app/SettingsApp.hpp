#pragma once
// Settings — brightness, theme, timezone. Persists via IStorage.
#include "yui/app/App.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/util/TzPresets.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>

#if !defined(YUI_TARGET_CARDPUTER_ADV)
#include <cstdlib>  // setenv on native
#include <ctime>    // tzset
#endif

namespace yui {

class SettingsApp : public App {
public:
  static constexpr int kBrightMin = 0;
  static constexpr int kBrightMax = 100;
  static constexpr int kBrightStep = 10;

  explicit SettingsApp(IStorage& store) : store_(store) {}
  const char* name() const override { return "Settings"; }

  void on_enter(Hal& /*hal*/) override {
    store_.init();
    int32_t v = 80;
    store_.get_int("brightness", v, 80);
    brightness_ = std::clamp(static_cast<int>(v), kBrightMin, kBrightMax);

    char tz_buf[40] = {0};
    if (!store_.get_str("clock.tz", tz_buf, sizeof(tz_buf)) || tz_buf[0] == 0) {
      std::strncpy(tz_buf, "UTC0", sizeof(tz_buf) - 1);
    }
    tz_idx_ = tz_index_of(tz_buf);
    cursor_ = 0;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Up   && cursor_ > 0) --cursor_;
    if (k.key == Key::Down && cursor_ + 1 < kRowCount) ++cursor_;
    if (k.key == Key::Left)  step_(-1);
    if (k.key == Key::Right) step_(+1);
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "Settings", kWhite, kJapanRed);

    char line[40];
    size_t tz_n = 0;
    const TzPreset* presets = tz_presets(tz_n);
    for (int i = 0; i < kRowCount; ++i) {
      const int y = 22 + i * 16;
      const bool sel = (i == cursor_);
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : kBlack;
      if (sel) d.fill_rect({0, y - 2, d.width(), 16}, bg);

      switch (i) {
        case 0:
          std::snprintf(line, sizeof(line), "Brightness   %3d%%", brightness_);
          break;
        case 1:
          std::snprintf(line, sizeof(line), "Timezone     %.12s",
                        presets[tz_idx_].label);
          break;
        case 2:
          std::snprintf(line, sizeof(line), "Theme        Hinomaru");
          break;
        default:
          std::snprintf(line, sizeof(line), "(coming soon)");
          break;
      }
      d.draw_text(8, y + 3, line, fg, bg);
    }

    d.draw_text(8, d.height() - 14, "<-/->  to change", kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  int    brightness() const { return brightness_; }
  int    cursor()     const { return cursor_; }
  size_t tz_index()   const { return tz_idx_; }
  const char* tz_posix() const {
    size_t n = 0;
    return tz_presets(n)[tz_idx_].posix;
  }

private:
  static constexpr int kRowCount = 3;

  void step_(int delta) {
    if (cursor_ == 0) {
      brightness_ = std::clamp(brightness_ + delta * kBrightStep,
                               kBrightMin, kBrightMax);
      store_.put_int("brightness", brightness_);
      return;
    }
    if (cursor_ == 1) {
      size_t n = 0;
      const TzPreset* p = tz_presets(n);
      // Cycle with wrap so Left at 0 lands on the last preset.
      const int next = (static_cast<int>(tz_idx_) + delta +
                        static_cast<int>(n)) % static_cast<int>(n);
      tz_idx_ = static_cast<size_t>(next);
      store_.put_str("clock.tz", p[tz_idx_].posix);
      apply_tz_native_(p[tz_idx_].posix);
      return;
    }
  }

  static void apply_tz_native_(const char* posix) {
#if !defined(YUI_TARGET_CARDPUTER_ADV)
    if (posix) {
      ::setenv("TZ", posix, /*overwrite=*/1);
      ::tzset();
    }
#else
    (void)posix;  // ESP32 path: ClockApp re-applies via configTzTime on resync.
#endif
  }

  IStorage& store_;
  int       brightness_ = 80;
  int       cursor_     = 0;
  size_t    tz_idx_     = 0;
};

}  // namespace yui

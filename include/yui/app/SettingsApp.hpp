#pragma once
// Settings — brightness, theme, timezone. Persists via IStorage.
#include "yui/app/App.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/util/TzPresets.hpp"
#include "yui/util/NtpPresets.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
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
  Category    category() const override { return Category::System; }

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

    char ntp_buf[40] = {0};
    if (!store_.get_str("clock.ntp", ntp_buf, sizeof(ntp_buf)) || ntp_buf[0] == 0) {
      std::strncpy(ntp_buf, "pool.ntp.org", sizeof(ntp_buf) - 1);
    }
    ntp_idx_ = ntp_index_of(ntp_buf);
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
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "Settings");

    char line[40];
    size_t tz_n = 0;
    const TzPreset* presets = tz_presets(tz_n);
    size_t ntp_n = 0;
    const NtpPreset* ntps = ntp_presets(ntp_n);
    for (int i = 0; i < kRowCount; ++i) {
      const int y = 22 + i * 16;
      const bool sel = (i == cursor_);
      const Color bg = sel ? ui::kAccent : ui::kSurface;
      const Color fg = sel ? ui::kSurface    : ui::kOnSurface;
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
          std::snprintf(line, sizeof(line), "NTP server   %.12s",
                        ntps[ntp_idx_].label);
          break;
        case 3:
          std::snprintf(line, sizeof(line), "Theme        Hinomaru");
          break;
        default:
          std::snprintf(line, sizeof(line), "(coming soon)");
          break;
      }
      d.draw_text(8, y + 3, line, fg, bg);
    }

    ui::Chrome::footer(d, "<-/->  to change");
    d.flush();
  }

  // Test hooks
  int    brightness() const { return brightness_; }
  int    cursor()     const { return cursor_; }
  size_t tz_index()   const { return tz_idx_; }
  size_t ntp_index()  const { return ntp_idx_; }
  const char* tz_posix() const {
    size_t n = 0;
    return tz_presets(n)[tz_idx_].posix;
  }
  const char* ntp_host() const {
    size_t n = 0;
    return ntp_presets(n)[ntp_idx_].host;
  }

private:
  static constexpr int kRowCount = 4;

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
      const int next = (static_cast<int>(tz_idx_) + delta +
                        static_cast<int>(n)) % static_cast<int>(n);
      tz_idx_ = static_cast<size_t>(next);
      store_.put_str("clock.tz", p[tz_idx_].posix);
      apply_tz_native_(p[tz_idx_].posix);
      return;
    }
    if (cursor_ == 2) {
      size_t n = 0;
      const NtpPreset* p = ntp_presets(n);
      const int next = (static_cast<int>(ntp_idx_) + delta +
                        static_cast<int>(n)) % static_cast<int>(n);
      ntp_idx_ = static_cast<size_t>(next);
      store_.put_str("clock.ntp", p[ntp_idx_].host);
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
  size_t    ntp_idx_    = 0;
};

}  // namespace yui

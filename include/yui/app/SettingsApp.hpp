#pragma once
// Settings — brightness + (later) volume, theme, etc. Persists via IStorage.
#include "yui/app/App.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <algorithm>

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
  int  brightness() const { return brightness_; }
  int  cursor() const { return cursor_; }

private:
  static constexpr int kRowCount = 2;

  void step_(int delta) {
    if (cursor_ != 0) return;  // only Brightness adjustable for v0.0
    brightness_ = std::clamp(brightness_ + delta * kBrightStep, kBrightMin, kBrightMax);
    store_.put_int("brightness", brightness_);
  }

  IStorage& store_;
  int       brightness_ = 80;
  int       cursor_     = 0;
};

}  // namespace yui

#pragma once
#include "yui/app/App.hpp"
#include "yui/app/AppRegistry.hpp"
#include "yui/shell/Menu.hpp"
#include "yui/types.hpp"

namespace yui {

// The Yui home screen — a vertical list of registered apps with a hinomaru
// header. Pure HAL, fully native-testable.
class Launcher : public App {
public:
  static constexpr int kHeaderH   = 16;
  static constexpr int kRowH      = 14;
  static constexpr int kRowPadX   = 8;
  static constexpr int kRowTextDy = 3;

  explicit Launcher(AppRegistry& reg) : reg_(reg), menu_(reg.size()) {}

  const char* name() const override { return "Yui"; }

  void on_enter(Hal& /*hal*/) override {
    menu_.set_count(reg_.size());
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    switch (k.key) {
      case Key::Up:    menu_.up();      break;
      case Key::Down:  menu_.down();    break;
      case Key::Enter: pending_launch_ = selected(); break;
      default: break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);

    // Hinomaru header bar
    d.fill_rect({0, 0, d.width(), kHeaderH}, kJapanRed);
    d.draw_text(kRowPadX, 4, "Yui", kWhite, kJapanRed);

    // App list
    for (std::size_t i = 0; i < reg_.size(); ++i) {
      const int y = kHeaderH + 4 + static_cast<int>(i) * kRowH;
      const bool selected = (i == menu_.cursor());
      const Color bg = selected ? kJapanRed     : kWhite;
      const Color fg = selected ? kWhite        : kJapanRed;
      if (selected) d.fill_rect({0, y - 2, d.width(), kRowH}, bg);
      const char* label = reg_.at(i) ? reg_.at(i)->name() : "(null)";
      d.draw_text(kRowPadX, y + kRowTextDy, label, fg, bg);
    }

    d.flush();
  }

  // Test / shell hooks
  std::size_t cursor() const { return menu_.cursor(); }
  App* selected() const { return reg_.at(menu_.cursor()); }
  App* take_pending_launch() {
    App* a = pending_launch_;
    pending_launch_ = nullptr;
    return a;
  }

private:
  AppRegistry& reg_;
  Menu menu_;
  App* pending_launch_ = nullptr;
};

}  // namespace yui

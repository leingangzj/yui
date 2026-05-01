#pragma once
#include "yui/app/App.hpp"
#include "yui/app/AppRegistry.hpp"
#include "yui/shell/Menu.hpp"
#include "yui/sys/SysProbe.hpp"
#include "yui/types.hpp"
#include <cstdio>

namespace yui {

// The Yui home screen — a vertical list of registered apps with a hinomaru
// header. Pure HAL, fully native-testable. An optional SysProbe lights up
// a battery / RSSI / uptime status strip on the right side of the header.
class Launcher : public App {
public:
  static constexpr int kHeaderH   = 16;
  static constexpr int kRowH      = 14;
  static constexpr int kRowPadX   = 8;
  static constexpr int kRowTextDy = 3;

  explicit Launcher(AppRegistry& reg) : reg_(reg), menu_(reg.size()) {}
  Launcher(AppRegistry& reg, SysProbe probe)
      : reg_(reg), menu_(reg.size()), probe_(std::move(probe)), have_probe_(true) {}

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

    // Status bar drawn last so it sits on top of any row that bleeds into
    // the header zone (it doesn't, but render order is robust this way).
    render_status_(d);

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
  void render_status_(IDisplay& d) {
    if (!have_probe_) return;
    char buf[32];
    int n = 0;
    if (probe_.battery_pct) {
      const int pct = probe_.battery_pct();
      if (pct >= 0)
        n += std::snprintf(buf + n, sizeof(buf) - n, "%d%% ", pct);
    }
    if (probe_.wifi_rssi) {
      const int rssi = probe_.wifi_rssi();
      if (rssi != 0)
        n += std::snprintf(buf + n, sizeof(buf) - n, "%d ", rssi);
    }
    if (probe_.uptime_ms) {
      const uint32_t up = probe_.uptime_ms();
      const uint32_t hh = up / 3600000u;
      const uint32_t mm = (up / 60000u) % 60u;
      n += std::snprintf(buf + n, sizeof(buf) - n, "%02u:%02u", hh, mm);
    }
    if (n == 0) return;
    // Roughly right-align: each char ~6 px in the M5 font.
    const int pixels = n * 6;
    const int x = d.width() - pixels - 4;
    d.draw_text(x, 4, buf, kWhite, kJapanRed);
  }

  AppRegistry& reg_;
  Menu menu_;
  App* pending_launch_ = nullptr;
  SysProbe probe_{};
  bool have_probe_ = false;
};

}  // namespace yui

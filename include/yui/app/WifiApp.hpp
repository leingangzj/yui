#pragma once
#include "yui/app/App.hpp"
#include "yui/hal/INet.hpp"
#include "yui/shell/Menu.hpp"
#include "yui/types.hpp"
#include <cstdio>

namespace yui {

class WifiApp : public App {
public:
  enum class State { Scanning, Done, Empty };

  explicit WifiApp(INet& net) : net_(net), menu_(0) {}

  const char* name() const override { return "WiFi"; }

  void on_enter(Hal& hal) override {
    hal_      = &hal;
    state_    = State::Scanning;
    last_poll_ = 0;
    net_.wifi_scan_start();
    menu_.set_count(0);
  }

  void tick(uint32_t now_ms) override {
    if (state_ != State::Scanning) return;
    if (now_ms - last_poll_ < 200 && last_poll_ != 0) return;
    last_poll_ = now_ms == 0 ? 1 : now_ms;
    if (net_.wifi_scan_ready()) {
      const size_t n = net_.wifi_count();
      menu_.set_count(n);
      state_ = (n == 0) ? State::Empty : State::Done;
    }
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (state_ != State::Done) {
      if (k.key == Key::Enter) {  // restart
        if (hal_) on_enter(*hal_);
      }
      return;
    }
    switch (k.key) {
      case Key::Up:    menu_.up();   break;
      case Key::Down:  menu_.down(); break;
      case Key::Enter:
        // Restart scan
        if (hal_) on_enter(*hal_);
        break;
      default: break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "WiFi Scan", kWhite, kJapanRed);

    if (state_ == State::Scanning) {
      d.draw_text(8, 40, "Scanning...", kJapanRed, kWhite);
      d.draw_text(8, 110, "Esc to go back", kJapanRedDark, kWhite);
      d.flush();
      return;
    }
    if (state_ == State::Empty) {
      d.draw_text(8, 40, "No networks found", kJapanRedDark, kWhite);
      d.draw_text(8, 60, "Enter to retry",   kJapanRed,     kWhite);
      d.draw_text(8, 110, "Esc to go back",   kJapanRedDark, kWhite);
      d.flush();
      return;
    }

    // Done — list with windowed scrolling.
    const size_t n      = net_.wifi_count();
    const size_t cur    = menu_.cursor();
    const size_t window = 7;
    const size_t start  = (cur >= window) ? (cur - window + 1) : 0;
    for (size_t i = start; i < n && i < start + window; ++i) {
      const auto& ap = net_.wifi_at(i);
      const int y    = 22 + static_cast<int>(i - start) * 13;
      const bool sel = (i == cur);
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : kBlack;
      if (sel) d.fill_rect({0, y - 2, d.width(), 13}, bg);

      char rssi_str[8];
      std::snprintf(rssi_str, sizeof(rssi_str), "%d", ap.rssi);

      const char* lock = ap.secured ? "*" : " ";
      char line[40];
      std::snprintf(line, sizeof(line), "%s%-20s %s", lock, ap.ssid, rssi_str);
      d.draw_text(4, y + 2, line, fg, bg);
    }
    d.flush();
  }

  // Test hooks
  State        state()  const { return state_; }
  std::size_t  cursor() const { return menu_.cursor(); }

private:
  INet& net_;
  Menu  menu_;
  Hal*  hal_       = nullptr;
  State state_     = State::Scanning;
  uint32_t last_poll_ = 0;
};

}  // namespace yui

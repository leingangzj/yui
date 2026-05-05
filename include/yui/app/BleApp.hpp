#pragma once
#include "yui/app/App.hpp"
#include "yui/hal/INet.hpp"
#include "yui/shell/Menu.hpp"
#include "yui/types.hpp"
#include <cstdio>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class BleApp : public App {
public:
  enum class State { Scanning, Done, Empty };
  static constexpr uint32_t kScanDurationMs = 4000;

  explicit BleApp(INet& net) : net_(net), menu_(0) {}

  const char* name() const override { return "BLE"; }
  Category    category() const override { return Category::Bluetooth; }
  const assets::IconRef* icon() const override { return &assets::icons::kScan(); }

  void on_enter(Hal& hal) override {
    hal_   = &hal;
    state_ = State::Scanning;
    net_.ble_scan_start(kScanDurationMs);
    menu_.set_count(0);
  }

  void tick(uint32_t /*now_ms*/) override {
    if (state_ != State::Scanning) return;
    if (net_.ble_scan_ready()) {
      const size_t n = net_.ble_count();
      menu_.set_count(n);
      state_ = (n == 0) ? State::Empty : State::Done;
    }
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (state_ != State::Done) {
      if (k.key == Key::Enter && hal_) on_enter(*hal_);
      return;
    }
    if (k.key == Key::Up)    menu_.up();
    if (k.key == Key::Down)  menu_.down();
    if (k.key == Key::Enter && hal_) on_enter(*hal_);
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "BLE Scan");

    if (state_ == State::Scanning) {
      d.draw_text(8, 40, "Listening...", ui::kAccent, ui::kSurface);
    ui::Chrome::footer(d, "Esc:back");
      d.flush();
      return;
    }
    if (state_ == State::Empty) {
      d.draw_text(8, 40, "No devices found", ui::kAccentDark, ui::kSurface);
      d.draw_text(8, 60, "Enter to retry",   ui::kAccent,     ui::kSurface);
      d.flush();
      return;
    }

    const size_t n      = net_.ble_count();
    const size_t cur    = menu_.cursor();
    const size_t window = 7;
    const size_t start  = (cur >= window) ? (cur - window + 1) : 0;
    for (size_t i = start; i < n && i < start + window; ++i) {
      const auto& dev = net_.ble_at(i);
      const int y     = 22 + static_cast<int>(i - start) * 13;
      const bool sel  = (i == cur);
      const Color bg = sel ? ui::kAccent : ui::kSurface;
      const Color fg = sel ? ui::kSurface    : ui::kOnSurface;
      if (sel) d.fill_rect({0, y - 2, d.width(), 13}, bg);

      const char* label = dev.name[0] ? dev.name : dev.addr;
      char line[40];
      std::snprintf(line, sizeof(line), "%-22s %d", label, dev.rssi);
      d.draw_text(4, y + 2, line, fg, bg);
    }
    d.flush();
  }

  State        state()  const { return state_; }
  std::size_t  cursor() const { return menu_.cursor(); }

private:
  INet& net_;
  Menu  menu_;
  Hal*  hal_   = nullptr;
  State state_ = State::Scanning;
};

}  // namespace yui

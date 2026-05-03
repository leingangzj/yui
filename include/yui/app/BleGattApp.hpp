#pragma once
// BleGattApp — connect to a BLE peripheral by MAC and walk its
// services + characteristics.
//
// Workflow:
//   Idle    — show MAC entry; Tab toggles characters; Enter connects
//   Services — list services; Up/Down cursor; Enter drills into chars
//   Chars    — list characteristics under selected service
//
// Built on IBleCentral; native fake answers from a registered service
// tree.
#include "yui/app/App.hpp"
#include "yui/hal/IBleCentral.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class BleGattApp : public App {
public:
  enum class View : uint8_t { Idle, Connecting, Services, Chars, Failed };

  static constexpr size_t kMaxServices = 16;
  static constexpr size_t kMaxChars    = 16;

  explicit BleGattApp(IBleCentral& central) : central_(central) {}

  const char* name() const override { return "GATT"; }
  Category    category() const override { return Category::Bluetooth; }

  void on_enter(Hal& hal) override {
    hal_      = &hal;
    view_     = View::Idle;
    services_count_ = 0;
    chars_count_    = 0;
    cursor_         = 0;
    if (mac_[0] == 0) std::strcpy(mac_, "00:00:00:00:00:00");
  }

  void on_exit() override { central_.disconnect(); }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (view_ == View::Idle || view_ == View::Failed) {
      if (k.key == Key::Enter) connect_();
      return;
    }
    if (view_ == View::Services) {
      if (k.key == Key::Up   && cursor_ > 0)                  --cursor_;
      if (k.key == Key::Down && cursor_ + 1 < services_count_) ++cursor_;
      if (k.key == Key::Enter) drill_();
      if (k.key == Key::Esc)   { /* Shell handles */ }
    } else if (view_ == View::Chars) {
      if (k.key == Key::Up   && cursor_ > 0)                --cursor_;
      if (k.key == Key::Down && cursor_ + 1 < chars_count_) ++cursor_;
      if (k.key == Key::Backspace) { view_ = View::Services; cursor_ = svc_cursor_; }
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "GATT");

    char line[40];
    if (view_ == View::Idle) {
      std::snprintf(line, sizeof(line), "MAC: %s", mac_);
      d.draw_text(8, 36, line, kBlack, kWhite);
      d.draw_text(8, 56, "Enter: connect", kJapanRed, kWhite);
    } else if (view_ == View::Connecting) {
      d.draw_text(8, 40, "Connecting...", kJapanRed, kWhite);
    } else if (view_ == View::Failed) {
      d.draw_text(8, 36, "Connect FAILED", kJapanRedBright, kWhite);
      d.draw_text(8, 56, "Enter: retry", kJapanRed, kWhite);
    } else if (view_ == View::Services) {
      const size_t window = 6;
      const size_t start  = (cursor_ >= window) ? (cursor_ - window + 1) : 0;
      for (size_t i = start; i < services_count_ && i < start + window; ++i) {
        const int y = 22 + static_cast<int>(i - start) * 16;
        const bool sel = (i == cursor_);
        const Color bg = sel ? kJapanRed : kWhite;
        const Color fg = sel ? kWhite    : kBlack;
        if (sel) d.fill_rect({0, y - 2, d.width(), 16}, bg);
        std::snprintf(line, sizeof(line), "S: %.32s", services_[i].uuid);
        d.draw_text(4, y + 3, line, fg, bg);
      }
    } else if (view_ == View::Chars) {
      for (size_t i = 0; i < chars_count_ && i < 6; ++i) {
        const int y = 22 + static_cast<int>(i) * 16;
        const bool sel = (i == cursor_);
        const Color bg = sel ? kJapanRed : kWhite;
        const Color fg = sel ? kWhite    : kBlack;
        if (sel) d.fill_rect({0, y - 2, d.width(), 16}, bg);
        std::snprintf(line, sizeof(line), "C: %.24s P:%02X",
                      chars_[i].uuid, chars_[i].properties);
        d.draw_text(4, y + 3, line, fg, bg);
      }
    }

    const char* hint =
        view_ == View::Services ? "Enter:open Esc:back" :
        view_ == View::Chars    ? "Bksp:back" :
                                  "Enter:connect Esc:back";
    d.draw_text(8, d.height() - 14, hint, kJapanRedDark, kWhite);
    d.flush();
  }

  // Configuration / test hooks
  void set_target_mac(const char* mac) {
    if (!mac) return;
    std::strncpy(mac_, mac, sizeof(mac_) - 1);
    mac_[sizeof(mac_) - 1] = '\0';
  }
  View   view()            const { return view_; }
  size_t services_count()  const { return services_count_; }
  size_t chars_count()     const { return chars_count_; }
  const GattService& service_at(size_t i) const { return services_[i]; }
  const GattCharacteristic& char_at(size_t i) const { return chars_[i]; }

private:
  void connect_() {
    view_ = View::Connecting;
    if (!central_.connect(mac_)) { view_ = View::Failed; return; }
    services_count_ = central_.enumerate_services(services_, kMaxServices);
    cursor_ = 0;
    view_ = View::Services;
  }

  void drill_() {
    if (cursor_ >= services_count_) return;
    svc_cursor_ = cursor_;
    chars_count_ = central_.enumerate_characteristics(
        services_[cursor_].uuid, chars_, kMaxChars);
    cursor_ = 0;
    view_   = View::Chars;
  }

  IBleCentral& central_;
  Hal*         hal_  = nullptr;
  View         view_ = View::Idle;
  char         mac_[18]    = {0};
  GattService  services_[kMaxServices];
  size_t       services_count_ = 0;
  GattCharacteristic chars_[kMaxChars];
  size_t       chars_count_    = 0;
  size_t       cursor_         = 0;
  size_t       svc_cursor_     = 0;
};

}  // namespace yui

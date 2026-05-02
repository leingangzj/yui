#pragma once
// RemoteApp — toggles the browser viewer server on/off and shows the URL
// to type into a desktop / phone browser. Lives under System.
//
// Two persistence levels:
//   • Runtime  (Enter)  — start/stop NOW, doesn't touch storage.
//   • Persist  (F)      — flip dev.remote NVS flag so the next boot
//                         either auto-starts or stays off.
//
// HAL-only so the native test env can drive it with a mock IRemote.

#include "yui/app/App.hpp"
#include "yui/hal/IRemote.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include <cstdio>

namespace yui {

inline constexpr const char* kStorageKeyDevRemote = "dev.remote";

class RemoteApp : public App {
public:
  RemoteApp(IRemote& remote, IStorage* store)
      : remote_(remote), store_(store) {}

  const char* name() const override { return "Remote VNC"; }
  Category    category() const override { return Category::System; }

  void on_enter(Hal& /*hal*/) override {
    if (store_) store_->get_int(kStorageKeyDevRemote, persist_, 0);
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    switch (k.key) {
      case Key::Enter:
        if (remote_.running()) remote_.stop();
        else                   remote_.start();
        break;
      case Key::Char:
        if (k.ch == 'f' || k.ch == 'F') {
          persist_ = persist_ ? 0 : 1;
          if (store_) store_->put_int(kStorageKeyDevRemote, persist_);
        }
        break;
      default: break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "Remote VNC", kWhite, kJapanRed);

    const bool on = remote_.running();
    d.draw_text(8,  26, on ? "Status: ON" : "Status: OFF",
                on ? kJapanRed : kJapanRedDark, kWhite);

    if (on) {
      char line[64];
      std::snprintf(line, sizeof(line), "http://%s/", remote_.ip());
      d.draw_text(8, 42, line, kBlack, kWhite);
      d.draw_text(8, 56, "open in any browser", kJapanRedDark, kWhite);
    } else {
      d.draw_text(8, 42, "(not advertising)", kJapanRedDark, kWhite);
    }

    d.draw_text(8,  82, persist_ ? "Auto-start: ON" : "Auto-start: OFF",
                persist_ ? kJapanRed : kJapanRedDark, kWhite);

    d.draw_text(8, 104, "Enter:toggle  F:auto-start", kJapanRedDark, kWhite);
    d.draw_text(8, 118, "Esc:back",                    kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  int  persist_flag() const { return persist_; }

private:
  IRemote&  remote_;
  IStorage* store_;
  int32_t   persist_ = 0;
};

}  // namespace yui

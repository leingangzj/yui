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
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
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
    d.clear(ui::kSurface);
    const bool on = remote_.running();
    ui::Chrome::header(d, "Remote VNC", on ? "ON" : "OFF");

    int y = ui::kBodyTopY;
    if (on) {
      char line[64];
      std::snprintf(line, sizeof(line), "http://%s/", remote_.ip());
      d.draw_text(ui::kBodyPadX, y, line, ui::kOnSurface, ui::kSurface);
      y += ui::kBodyLineH;
      d.draw_text(ui::kBodyPadX, y, "open in any browser",
                  ui::kHint, ui::kSurface);
    } else {
      d.draw_text(ui::kBodyPadX, y, "(not advertising)",
                  ui::kHint, ui::kSurface);
    }
    y += ui::kBodyLineH + 6;

    d.draw_text(ui::kBodyPadX, y,
                persist_ ? "Auto-start: ON" : "Auto-start: OFF",
                persist_ ? ui::kAccent : ui::kHint, ui::kSurface);

    ui::Chrome::footer(d, "Enter:toggle  F:auto  Esc:back");
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

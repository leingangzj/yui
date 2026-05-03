#pragma once
// KarmaApp — toggle the Pineapple's Karma (probe-responder) feature.
// Pairs naturally with EvilTwin: Karma makes the rogue AP respond to
// any SSID a nearby client probes for.
//
// LEGAL: same caveat as EvilTwin — only on networks/devices you own
// or have written authorization to test.
#include "yui/app/App.hpp"
#include "yui/proto/Pineapple.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include <cstdio>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class KarmaApp : public App {
public:
  KarmaApp(IHttp& http, IStorage& store)
      : client_(http, &store), store_(store) {}

  const char* name() const override { return "Karma"; }
  Category    category() const override { return Category::WiFi; }

  void on_enter(Hal& hal) override {
    hal_ = &hal;
    enabled_ = false;
    last_ok_ = false;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Enter && k.fn) {
      last_ok_ = ensure_authed_() && client_.pineap_set_enabled(true, true);
      enabled_ = last_ok_;
    } else if (k.key == Key::Backspace) {
      last_ok_ = ensure_authed_() && client_.pineap_set_enabled(true, false);
      if (last_ok_) enabled_ = false;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "Karma (PA)");

    char line[40];
    std::snprintf(line, sizeof(line), "Karma: %s", enabled_ ? "ON" : "off");
    d.draw_text(8, 24, line, enabled_ ? ui::kWarn : ui::kOnSurface, ui::kSurface);
    d.draw_text(8, 44, "Fn+Enter: ENABLE", ui::kAccent,     ui::kSurface);
    d.draw_text(8, 60, "Backspace: disable", ui::kAccentDark, ui::kSurface);
    if (last_ok_)  d.draw_text(8, 80, "Last action: ok",   ui::kAccent,        ui::kSurface);
    if (!last_ok_ && enabled_) d.draw_text(8, 80, "Last action: FAILED",
                                            ui::kWarn, ui::kSurface);
    ui::Chrome::footer(d, "Use only on YOUR network");
    d.flush();
  }

  // Test hooks
  bool enabled() const { return enabled_; }
  bool last_ok() const { return last_ok_; }

private:
  bool ensure_authed_() {
    if (client_.authenticated()) return true;
    char host[64] = {0};
    if (!store_.get_str("pa.host", host, sizeof(host)) || host[0] == 0) return false;
    int32_t port = 1471;
    store_.get_int("pa.port", port, 1471);
    char user[40] = {0}, pass[40] = {0};
    store_.get_str("pa.user", user, sizeof(user));
    store_.get_str("pa.pass", pass, sizeof(pass));
    return client_.login(host, static_cast<uint16_t>(port), user, pass);
  }

  pineapple::Client client_;
  IStorage&         store_;
  Hal*              hal_     = nullptr;
  bool              enabled_ = false;
  bool              last_ok_ = false;
};

}  // namespace yui

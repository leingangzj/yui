#pragma once
// EvilTwinApp — Pineapple PineAP rogue-AP toggle + SSID pool editor.
// v0.2 stretch.
//
// LEGAL: USE ONLY ON NETWORKS / DEVICES YOU OWN OR HAVE WRITTEN
// AUTHORIZATION TO TEST. The app gates "Enable" behind a held-key
// confirmation (Fn+Enter) to avoid casual misuse.
#include "yui/app/App.hpp"
#include "yui/proto/Pineapple.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class EvilTwinApp : public App {
public:
  EvilTwinApp(IHttp& http, IStorage& store)
      : client_(http, &store), store_(store) {}

  const char* name() const override { return "EvilTwin"; }
  Category    category() const override { return Category::WiFi; }

  void on_enter(Hal& hal) override {
    hal_ = &hal;
    enabled_ = false;
    last_action_ = Action::None;
    last_ok_ = false;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Enter && k.fn) {
      // Held-key confirmation enables PineAP.
      last_ok_ = ensure_authed_() && client_.pineap_set_enabled(true, false);
      enabled_ = last_ok_;
      last_action_ = Action::Enable;
    } else if (k.key == Key::Backspace) {
      last_ok_ = ensure_authed_() && client_.pineap_set_enabled(false, false);
      enabled_ = !last_ok_ ? enabled_ : false;
      last_action_ = Action::Disable;
    } else if (k.key == Key::Tab) {
      // Wipe SSID pool.
      last_ok_ = ensure_authed_() && client_.pineap_clear_ssids();
      last_action_ = Action::Clear;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "PA Evil Twin");

    char line[40];
    int y = 22;
    std::snprintf(line, sizeof(line), "PineAP: %s",
                  enabled_ ? "ON" : "off");
    d.draw_text(8, y, line, enabled_ ? kJapanRedBright : kBlack, kWhite); y += 14;

    d.draw_text(8, y, "Fn+Enter: ENABLE",  kJapanRed,     kWhite); y += 14;
    d.draw_text(8, y, "Bksp:     disable", kJapanRedDark, kWhite); y += 14;
    d.draw_text(8, y, "Tab:      clear pool", kJapanRedDark, kWhite); y += 14;

    if (last_action_ != Action::None) {
      const char* a = last_action_ == Action::Enable  ? "Enable" :
                      last_action_ == Action::Disable ? "Disable":
                                                        "Clear";
      std::snprintf(line, sizeof(line), "%s: %s", a, last_ok_ ? "ok" : "FAIL");
      d.draw_text(8, y, line, last_ok_ ? kJapanRed : kJapanRedBright, kWhite);
    }

    ui::Chrome::footer(d, "Use only on YOUR network");
    d.flush();
  }

  // Test hooks
  bool enabled() const { return enabled_; }
  bool last_ok() const { return last_ok_; }

private:
  enum class Action { None, Enable, Disable, Clear };

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
  Hal*              hal_         = nullptr;
  bool              enabled_     = false;
  Action            last_action_ = Action::None;
  bool              last_ok_     = false;
};

}  // namespace yui

#pragma once
// HandshakeBrowserApp — quick view of WPA handshakes captured by the
// Pineapple. We don't decode the whole (variable) shape; we just
// count BSSID occurrences in the raw JSON body so the user can see
// "yes, n handshakes are sitting on the device."
//
// Real exploration / download remains on the Pineapple's web UI.
#include "yui/app/App.hpp"
#include "yui/proto/Pineapple.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class HandshakeBrowserApp : public App {
public:
  HandshakeBrowserApp(IHttp& http, IStorage& store)
      : client_(http, &store), store_(store) {}

  const char* name() const override { return "Handshakes"; }
  Category    category() const override { return Category::WiFi; }

  void on_enter(Hal& hal) override {
    hal_ = &hal;
    refresh_();
  }

  void on_key(KeyEvent k) override {
    if (k.down && k.key == Key::Tab)   refresh_();
    if (k.down && k.key == Key::Enter) refresh_();
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "Handshakes (PA)");

    char line[40];
    if (!last_ok_) {
      d.draw_text(8, 40, "Failed to fetch", ui::kWarn, ui::kSurface);
    } else {
      std::snprintf(line, sizeof(line), "Captured: %u", static_cast<unsigned>(count_));
      d.draw_text(8, 40, line, ui::kOnSurface, ui::kSurface);
      d.draw_text(8, 60, "(Use PA web UI to view)", ui::kAccentDark, ui::kSurface);
    }
    ui::Chrome::footer(d, "Tab:refresh Esc:back");
    d.flush();
  }

  // Test hooks
  bool   last_ok() const { return last_ok_; }
  size_t count()   const { return count_; }
  pineapple::Client& client() { return client_; }

private:
  void refresh_() {
    if (!ensure_authed_()) { last_ok_ = false; count_ = 0; return; }
    char body[2048] = {0};
    if (!client_.handshakes_raw(body, sizeof(body))) {
      last_ok_ = false; count_ = 0; return;
    }
    last_ok_ = true;
    count_ = 0;
    const char* p = body;
    while ((p = std::strstr(p, "\"bssid\"")) != nullptr) {
      ++count_;
      ++p;
    }
  }

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
  bool              last_ok_ = false;
  size_t            count_   = 0;
};

}  // namespace yui

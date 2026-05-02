#pragma once
// WifiDeauthApp — Pineapple-driven deauth. We do NOT TX deauth from
// the Cardputer (per protocol audit; Espressif blocks it without
// patched libnet). The Pineapple does the radio work; the Cardputer
// drives.
//
// Strict legal gate: Fn+Enter required to fire. Default state is
// "armed=false" so accidentally-pressing Enter does nothing.
//
// User configures target BSSID + channel via Up/Down on each field.
// (Channel is settable directly; BSSID would normally come from the
// Recon results in v0.3 — for v0.2 stretch the user types it.)
#include "yui/app/App.hpp"
#include "yui/proto/Pineapple.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class WifiDeauthApp : public App {
public:
  WifiDeauthApp(IHttp& http, IStorage& store)
      : client_(http, &store), store_(store) {}

  const char* name() const override { return "Deauth"; }
  Category    category() const override { return Category::WiFi; }

  void on_enter(Hal& hal) override {
    hal_   = &hal;
    armed_ = false;
    fired_ = false;
    last_ok_ = false;
    // Default placeholder — user edits before firing.
    std::strncpy(bssid_, "00:00:00:00:00:00", sizeof(bssid_) - 1);
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Tab)   armed_ = !armed_;
    if (k.key == Key::Up   && channel_ < 13) ++channel_;
    if (k.key == Key::Down && channel_ > 1)  --channel_;
    if (k.key == Key::Enter && k.fn && armed_) fire_();
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "Deauth (via PA)", kWhite, kJapanRed);

    char line[40];
    int y = 22;
    std::snprintf(line, sizeof(line), "BSSID: %s", bssid_);
    d.draw_text(8, y, line, kBlack, kWhite); y += 14;
    std::snprintf(line, sizeof(line), "Channel: %u",
                  static_cast<unsigned>(channel_));
    d.draw_text(8, y, line, kBlack, kWhite); y += 14;
    std::snprintf(line, sizeof(line), "Armed: %s", armed_ ? "YES" : "no");
    d.draw_text(8, y, line, armed_ ? kJapanRedBright : kJapanRedDark, kWhite);
    y += 14;
    if (fired_) {
      d.draw_text(8, y, last_ok_ ? "Last fire: ok" : "Last fire: FAIL",
                  last_ok_ ? kJapanRed : kJapanRedBright, kWhite);
    }
    d.draw_text(8, d.height() - 14,
                "Tab:arm Fn+Enter:fire (own net only)",
                kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  bool armed()    const { return armed_; }
  bool fired()    const { return fired_; }
  bool last_ok()  const { return last_ok_; }
  void set_target(const char* b, uint8_t ch) {
    std::strncpy(bssid_, b ? b : "", sizeof(bssid_) - 1);
    bssid_[sizeof(bssid_) - 1] = '\0';
    channel_ = ch;
  }

private:
  void fire_() {
    fired_ = true;
    if (!ensure_authed_()) { last_ok_ = false; return; }
    last_ok_ = client_.deauth_ap(bssid_, channel_, 1);
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
  Hal*              hal_       = nullptr;
  char              bssid_[18] = {0};
  uint8_t           channel_   = 6;
  bool              armed_     = false;
  bool              fired_     = false;
  bool              last_ok_   = false;
};

}  // namespace yui

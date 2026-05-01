#pragma once
// PineappleApp — connection + dashboard view for the WiFi Pineapple
// Mark VII. v0.2 Track B must-have.
//
// On enter: pulls saved host/port/user/pass from NVS, logs in, fetches
// dashboard cards. Tab triggers a fresh fetch.
//
// CRITICAL UX NOTE (per docs/protocols/PINEAPPLE_API.md): the
// Cardputer must associate to the Pineapple's WiFi (different SSID
// from home) for any of this to work. v0.2 surfaces that as part of
// the "no host configured" hint string. The actual network swap is
// handled by main.cpp / future setup flow.
#include "yui/app/App.hpp"
#include "yui/proto/Pineapple.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class PineappleApp : public App {
public:
  PineappleApp(IHttp& http, IStorage& store)
      : client_(http, &store), store_(store) {}

  const char* name() const override { return "Pineapple"; }
  Category    category() const override { return Category::WiFi; }

  void on_enter(Hal& hal) override {
    hal_ = &hal;
    last_ok_ = false;
    err_msg_[0] = '\0';
    refresh_();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Tab)   refresh_();
    if (k.key == Key::Enter) refresh_();
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "Pineapple", kWhite, kJapanRed);

    char line[40];
    int y = 22;
    if (!host_loaded_) {
      d.draw_text(8, y, "No host configured", kJapanRedDark, kWhite);
      d.draw_text(8, d.height() - 14, "Set pa.host in NVS  Esc:back",
                  kJapanRedDark, kWhite);
      d.flush();
      return;
    }
    std::snprintf(line, sizeof(line), "Host: %.30s", host_);
    d.draw_text(8, y, line, kBlack, kWhite); y += 14;

    if (!last_ok_) {
      std::snprintf(line, sizeof(line), "Error: %.28s",
                    err_msg_[0] ? err_msg_ : "unreachable");
      d.draw_text(8, y, line, kJapanRedBright, kWhite);
    } else {
      std::snprintf(line, sizeof(line), "CPU:%3d%%   Mem:%3d%%",
                    cards_.cpu_pct, cards_.mem_pct);
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
      std::snprintf(line, sizeof(line), "Temp: %dC", cards_.temp_c);
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
      std::snprintf(line, sizeof(line), "Clients: %d",
                    cards_.clients_connected);
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
      std::snprintf(line, sizeof(line), "SSIDs: %d", cards_.total_ssids);
      d.draw_text(8, y, line, kBlack, kWhite);
    }
    d.draw_text(8, d.height() - 14, "Tab:refresh  Esc:back",
                kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  bool                 last_ok() const { return last_ok_; }
  const pineapple::Cards& cards() const { return cards_; }
  pineapple::Client&   client() { return client_; }

private:
  void refresh_() {
    if (!load_creds_()) { host_loaded_ = false; return; }
    host_loaded_ = true;
    if (!client_.authenticated()) {
      if (!client_.login(host_, port_, user_, pass_)) {
        last_ok_ = false;
        std::strncpy(err_msg_, "login failed", sizeof(err_msg_) - 1);
        return;
      }
    }
    last_ok_ = client_.dashboard_cards(cards_);
    if (!last_ok_) std::strncpy(err_msg_, "fetch failed", sizeof(err_msg_) - 1);
  }

  bool load_creds_() {
    char host[64] = {0};
    if (!store_.get_str("pa.host", host, sizeof(host)) || host[0] == 0) return false;
    int32_t port = 1471;
    store_.get_int("pa.port", port, 1471);
    char user[40] = {0};
    char pass[40] = {0};
    store_.get_str("pa.user", user, sizeof(user));
    store_.get_str("pa.pass", pass, sizeof(pass));
    std::strncpy(host_, host, sizeof(host_) - 1);
    std::strncpy(user_, user, sizeof(user_) - 1);
    std::strncpy(pass_, pass, sizeof(pass_) - 1);
    port_ = static_cast<uint16_t>(port);
    return true;
  }

  pineapple::Client client_;
  IStorage&         store_;
  Hal*              hal_         = nullptr;
  pineapple::Cards  cards_{};
  char              host_[64]    = {0};
  char              user_[40]    = {0};
  char              pass_[40]    = {0};
  uint16_t          port_        = 1471;
  bool              host_loaded_ = false;
  bool              last_ok_     = false;
  char              err_msg_[32] = {0};
};

}  // namespace yui

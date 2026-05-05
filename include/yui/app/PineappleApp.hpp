#pragma once
// PineappleApp — unified WiFi Pineapple workbench. Tab cycles three
// views that all sit on top of the same pineapple::Client and shared
// credentials in NVS:
//
//   Dashboard  — system cards (CPU/mem/temp/clients/SSIDs)
//   Recon      — start/poll a recon scan (Idle/Starting/Scanning/Done/Error)
//   Handshakes — count handshakes captured on the device
//
// The Cardputer must be associated to the Pineapple's WiFi network for
// any of this to work; the "no host configured" hint in Dashboard says
// so explicitly. Network association is handled outside this app.
#include "yui/app/App.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/proto/Pineapple.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class PineappleApp : public App {
 public:
  enum class Tab { Dashboard, Recon, Handshakes };
  enum class ReconState : uint8_t { Idle, Starting, Scanning, Done, Error };

  PineappleApp(IHttp& http, IStorage& store)
      : client_(http, &store), store_(store) {}

  const char* name() const override { return "Pineapple"; }
  Category    category() const override { return Category::WiFi; }

  void on_enter(Hal& hal) override {
    hal_ = &hal;
    refresh_dashboard_();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Tab) {
      cycle_tab_();
      return;
    }
    switch (tab_) {
      case Tab::Dashboard:
        if (k.key == Key::Enter) refresh_dashboard_();
        break;
      case Tab::Recon:
        if (k.key == Key::Enter &&
            (recon_state_ == ReconState::Idle ||
             recon_state_ == ReconState::Done ||
             recon_state_ == ReconState::Error)) {
          start_scan_();
        }
        break;
      case Tab::Handshakes:
        if (k.key == Key::Enter) refresh_handshakes_();
        break;
    }
  }

  void tick(uint32_t now_ms) override {
    if (tab_ != Tab::Recon || recon_state_ != ReconState::Scanning) return;
    if (now_ms - last_poll_ms_ < kPollIntervalMs && last_poll_ms_ != 0) return;
    last_poll_ms_ = now_ms == 0 ? 1 : now_ms;
    int sid = 0; bool running = false; int pct = 0;
    if (!client_.recon_status(sid, running, pct)) {
      recon_state_ = ReconState::Error;
      std::strncpy(err_msg_, "status fetch failed", sizeof(err_msg_) - 1);
      return;
    }
    recon_percent_ = pct;
    if (!running) recon_state_ = ReconState::Done;
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    const char* title = tab_ == Tab::Dashboard ? "Pineapple"
                      : tab_ == Tab::Recon     ? "Pineapple Recon"
                                               : "Handshakes (PA)";
    ui::Chrome::header(d, title);

    char line[40];
    int y = 22;
    switch (tab_) {
      case Tab::Dashboard:
        if (!host_loaded_) {
          d.draw_text(8, y, "No host configured", ui::kAccentDark, ui::kSurface);
          break;
        }
        std::snprintf(line, sizeof(line), "Host: %.30s", host_);
        d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 14;
        if (!dash_ok_) {
          std::snprintf(line, sizeof(line), "Error: %.28s",
                        err_msg_[0] ? err_msg_ : "unreachable");
          d.draw_text(8, y, line, ui::kWarn, ui::kSurface);
        } else {
          std::snprintf(line, sizeof(line), "CPU:%3d%%   Mem:%3d%%",
                        cards_.cpu_pct, cards_.mem_pct);
          d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 14;
          std::snprintf(line, sizeof(line), "Temp: %dC", cards_.temp_c);
          d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 14;
          std::snprintf(line, sizeof(line), "Clients: %d",
                        cards_.clients_connected);
          d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 14;
          std::snprintf(line, sizeof(line), "SSIDs: %d", cards_.total_ssids);
          d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface);
        }
        break;
      case Tab::Recon:
        switch (recon_state_) {
          case ReconState::Idle:
            d.draw_text(8, y, "Press Enter to scan",
                        ui::kAccentDark, ui::kSurface);
            break;
          case ReconState::Starting:
            d.draw_text(8, y, "Starting scan...", ui::kAccent, ui::kSurface);
            break;
          case ReconState::Scanning: {
            std::snprintf(line, sizeof(line), "Scanning: %d%%", recon_percent_);
            d.draw_text(8, y, line, ui::kAccent, ui::kSurface); y += 14;
            const int bar_w = 200;
            const int filled = (bar_w * recon_percent_) / 100;
            d.fill_rect({8, y, bar_w, 8}, ui::kSurface);
            if (filled > 0) d.fill_rect({8, y, filled, 8}, ui::kAccent);
            break;
          }
          case ReconState::Done:
            std::snprintf(line, sizeof(line), "Scan %d complete", recon_scan_id_);
            d.draw_text(8, y, line, ui::kAccent, ui::kSurface); y += 14;
            d.draw_text(8, y, "(Results on Pineapple UI)",
                        ui::kAccentDark, ui::kSurface);
            break;
          case ReconState::Error:
            d.draw_text(8, y, "Error:", ui::kWarn, ui::kSurface); y += 14;
            d.draw_text(8, y, err_msg_, ui::kAccentDark, ui::kSurface);
            break;
        }
        break;
      case Tab::Handshakes:
        if (!hs_ok_) {
          d.draw_text(8, y + 14, "Failed to fetch", ui::kWarn, ui::kSurface);
        } else {
          std::snprintf(line, sizeof(line), "Captured: %u",
                        static_cast<unsigned>(hs_count_));
          d.draw_text(8, y + 14, line, ui::kOnSurface, ui::kSurface);
          d.draw_text(8, y + 32, "(Use PA web UI to view)",
                      ui::kAccentDark, ui::kSurface);
        }
        break;
    }
    ui::Chrome::footer(d, "Tab:cycle  Enter:refresh  Esc:back");
    d.flush();
  }

  // Test hooks
  Tab        tab()           const { return tab_; }
  void       set_tab(Tab t)        { tab_ = t; on_tab_change_(); }
  // Dashboard
  bool                 last_ok() const { return dash_ok_; }
  const pineapple::Cards& cards() const { return cards_; }
  // Recon
  ReconState state()         const { return recon_state_; }
  int        scan_id()       const { return recon_scan_id_; }
  int        percent()       const { return recon_percent_; }
  // Handshakes
  size_t     count()         const { return hs_count_; }
  bool       handshakes_ok() const { return hs_ok_; }
  // Shared
  pineapple::Client& client() { return client_; }

 private:
  static constexpr uint32_t kPollIntervalMs = 1000;

  void cycle_tab_() {
    tab_ = tab_ == Tab::Dashboard ? Tab::Recon
         : tab_ == Tab::Recon     ? Tab::Handshakes
                                  : Tab::Dashboard;
    on_tab_change_();
  }

  void on_tab_change_() {
    if      (tab_ == Tab::Dashboard)  refresh_dashboard_();
    else if (tab_ == Tab::Handshakes) refresh_handshakes_();
    else if (tab_ == Tab::Recon &&
             recon_state_ == ReconState::Scanning) {
      // already in flight — leave alone
    }
  }

  void refresh_dashboard_() {
    err_msg_[0] = '\0';
    if (!load_creds_()) { host_loaded_ = false; dash_ok_ = false; return; }
    host_loaded_ = true;
    if (!ensure_authed_()) {
      dash_ok_ = false;
      std::strncpy(err_msg_, "login failed", sizeof(err_msg_) - 1);
      return;
    }
    dash_ok_ = client_.dashboard_cards(cards_);
    if (!dash_ok_) std::strncpy(err_msg_, "fetch failed", sizeof(err_msg_) - 1);
  }

  void refresh_handshakes_() {
    if (!ensure_authed_()) { hs_ok_ = false; hs_count_ = 0; return; }
    char body[2048] = {0};
    if (!client_.handshakes_raw(body, sizeof(body))) {
      hs_ok_ = false; hs_count_ = 0; return;
    }
    hs_ok_   = true;
    hs_count_ = 0;
    const char* p = body;
    while ((p = std::strstr(p, "\"bssid\"")) != nullptr) { ++hs_count_; ++p; }
  }

  void start_scan_() {
    recon_state_  = ReconState::Starting;
    last_poll_ms_ = 0;
    recon_percent_ = 0;
    if (!ensure_authed_()) {
      recon_state_ = ReconState::Error;
      std::strncpy(err_msg_, "login failed", sizeof(err_msg_) - 1);
      return;
    }
    int sid = 0;
    if (!client_.recon_start(true, 30, "2.4ghz", sid)) {
      recon_state_ = ReconState::Error;
      std::strncpy(err_msg_, "scan start failed", sizeof(err_msg_) - 1);
      return;
    }
    recon_scan_id_ = sid;
    recon_state_   = ReconState::Scanning;
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

  bool ensure_authed_() {
    if (client_.authenticated()) return true;
    if (!load_creds_()) return false;
    return client_.login(host_, port_, user_, pass_);
  }

  pineapple::Client client_;
  IStorage&         store_;
  Hal*              hal_  = nullptr;
  Tab               tab_  = Tab::Dashboard;
  // Dashboard
  pineapple::Cards  cards_{};
  bool              host_loaded_  = false;
  bool              dash_ok_      = false;
  char              host_[64]     = {0};
  char              user_[40]     = {0};
  char              pass_[40]     = {0};
  uint16_t          port_         = 1471;
  // Recon
  ReconState        recon_state_  = ReconState::Idle;
  int               recon_scan_id_ = 0;
  int               recon_percent_ = 0;
  uint32_t          last_poll_ms_  = 0;
  // Handshakes
  bool              hs_ok_   = false;
  size_t            hs_count_ = 0;
  // Shared
  char              err_msg_[32] = {0};
};

}  // namespace yui

#pragma once
// WiFi app — scan, pick an AP, type the passphrase, associate, persist.
//
// State flow:
//   Scanning → Done (or Empty)
//   Done   : Up/Down move cursor, Enter selects → EnterPass, Tab rescans
//   Empty  : Enter rescans
//   EnterPass: printable chars append, Backspace deletes, Enter submits →
//              Connecting, Esc returns to Done
//   Connecting → Connected or Failed (driven by INet::wifi_state)
//   Connected/Failed: Enter returns to Done (Failed re-opens EnterPass with
//                    the same SSID so the user can fix a typo)
//
// Stored keys (when an IStorage is provided):
//   wifi.ssid, wifi.pass — read at boot for auto-connect.
#include "yui/app/App.hpp"
#include "yui/hal/INet.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/shell/Menu.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class WifiApp : public App {
public:
  enum class State {
    Scanning,
    Done,
    Empty,
    EnterPass,
    Connecting,
    Connected,
    Failed,
  };

  static constexpr size_t kMaxPass = 64;

  explicit WifiApp(INet& net, IStorage* store = nullptr)
      : net_(net), store_(store), menu_(0) {}

  const char* name() const override { return "WiFi"; }

  void on_enter(Hal& hal) override {
    hal_       = &hal;
    last_poll_ = 0;
    // If the device is already connected (e.g. boot auto-connect), show
    // that instead of spinning up a fresh scan.
    if (net_.wifi_state() == WifiState::Connected) {
      state_ = State::Connected;
      std::strncpy(sel_ssid_, "(saved)", sizeof(sel_ssid_) - 1);
      return;
    }
    state_ = State::Scanning;
    pass_len_ = 0;
    pass_[0]  = 0;
    sel_ssid_[0] = 0;
    sel_secured_ = false;
    net_.wifi_scan_start();
    menu_.set_count(0);
  }

  void tick(uint32_t now_ms) override {
    if (state_ == State::Scanning) {
      if (now_ms - last_poll_ < 200 && last_poll_ != 0) return;
      last_poll_ = now_ms == 0 ? 1 : now_ms;
      if (net_.wifi_scan_ready()) {
        const size_t n = net_.wifi_count();
        menu_.set_count(n);
        state_ = (n == 0) ? State::Empty : State::Done;
      }
      return;
    }
    if (state_ == State::Connecting) {
      const WifiState s = net_.wifi_state();
      if (s == WifiState::Connected) state_ = State::Connected;
      else if (s == WifiState::Failed) state_ = State::Failed;
      return;
    }
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    switch (state_) {
      case State::Scanning:                                       break;
      case State::Empty:     handle_empty_(k);                    break;
      case State::Done:      handle_done_(k);                     break;
      case State::EnterPass: handle_pass_(k);                     break;
      case State::Connecting:                                     break;
      case State::Connected:
      case State::Failed:    handle_terminal_(k);                 break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "WiFi", kWhite, kJapanRed);

    switch (state_) {
      case State::Scanning:   render_scanning_(d);   break;
      case State::Empty:      render_empty_(d);      break;
      case State::Done:       render_list_(d);       break;
      case State::EnterPass:  render_pass_(d);       break;
      case State::Connecting: render_connecting_(d); break;
      case State::Connected:  render_connected_(d);  break;
      case State::Failed:     render_failed_(d);     break;
    }
    d.flush();
  }

  // Test hooks
  State        state()    const { return state_; }
  std::size_t  cursor()   const { return menu_.cursor(); }
  const char*  selected_ssid() const { return sel_ssid_; }
  const char*  pass_buffer()   const { return pass_; }

private:
  // ── input handlers ────────────────────────────────────────────────────
  void handle_empty_(KeyEvent k) {
    if (k.key == Key::Enter && hal_) on_enter(*hal_);
  }

  void handle_done_(KeyEvent k) {
    if (k.key == Key::Backspace && k.fn) { forget_(); return; }
    switch (k.key) {
      case Key::Up:    menu_.up();           break;
      case Key::Down:  menu_.down();         break;
      case Key::Tab:
        if (hal_) on_enter(*hal_);  // rescan
        break;
      case Key::Enter: select_current_();    break;
      default: break;
    }
  }

  void handle_pass_(KeyEvent k) {
    if (k.key == Key::Esc) { state_ = State::Done; return; }
    if (k.key == Key::Backspace) {
      if (pass_len_ > 0) { --pass_len_; pass_[pass_len_] = 0; }
      return;
    }
    if (k.key == Key::Enter) {
      submit_();
      return;
    }
    if (k.key == Key::Space && pass_len_ + 1 < kMaxPass) {
      pass_[pass_len_++] = ' ';
      pass_[pass_len_]   = 0;
      return;
    }
    if (k.key == Key::Char && k.ch >= 0x20 && k.ch < 0x7F &&
        pass_len_ + 1 < kMaxPass) {
      pass_[pass_len_++] = k.ch;
      pass_[pass_len_]   = 0;
    }
  }

  void handle_terminal_(KeyEvent k) {
    if (k.key == Key::Backspace && k.fn) { forget_(); return; }
    if (k.key != Key::Enter) return;
    if (state_ == State::Failed) {
      // Re-open the editor so the user can fix a typo.
      state_ = State::EnterPass;
    } else {
      // Connected → back to list (kept around for re-pick).
      state_ = State::Done;
    }
  }

  void forget_() {
    if (store_) {
      store_->erase("wifi.ssid");
      store_->erase("wifi.pass");
    }
    net_.wifi_disconnect();
    sel_ssid_[0] = 0;
    pass_[0] = 0;
    pass_len_ = 0;
    if (hal_) on_enter(*hal_);  // re-scan from a clean slate
  }

  // ── transitions ───────────────────────────────────────────────────────
  void select_current_() {
    if (net_.wifi_count() == 0) return;
    const auto& ap = net_.wifi_at(menu_.cursor());
    std::strncpy(sel_ssid_, ap.ssid, sizeof(sel_ssid_) - 1);
    sel_ssid_[sizeof(sel_ssid_) - 1] = 0;
    sel_secured_ = ap.secured;
    pass_len_    = 0;
    pass_[0]     = 0;
    if (!sel_secured_) {
      submit_();  // open networks: skip the editor
    } else {
      state_ = State::EnterPass;
    }
  }

  void submit_() {
    if (sel_ssid_[0] == 0) return;
    net_.wifi_connect(sel_ssid_, pass_);
    if (store_) {
      store_->put_str("wifi.ssid", sel_ssid_);
      store_->put_str("wifi.pass", pass_);
    }
    state_ = State::Connecting;
    last_poll_ = 0;
  }

  // ── rendering ─────────────────────────────────────────────────────────
  void render_scanning_(IDisplay& d) {
    d.draw_text(8, 40,  "Scanning...",  kJapanRed,     kWhite);
    d.draw_text(8, 110, "Esc to go back", kJapanRedDark, kWhite);
  }

  void render_empty_(IDisplay& d) {
    d.draw_text(8, 40,  "No networks found", kJapanRedDark, kWhite);
    d.draw_text(8, 60,  "Enter to retry",    kJapanRed,     kWhite);
    d.draw_text(8, 110, "Esc to go back",    kJapanRedDark, kWhite);
  }

  void render_list_(IDisplay& d) {
    const size_t n      = net_.wifi_count();
    const size_t cur    = menu_.cursor();
    const size_t window = 7;
    const size_t start  = (cur >= window) ? (cur - window + 1) : 0;
    for (size_t i = start; i < n && i < start + window; ++i) {
      const auto& ap = net_.wifi_at(i);
      const int y    = 22 + static_cast<int>(i - start) * 13;
      const bool sel = (i == cur);
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : kBlack;
      if (sel) d.fill_rect({0, y - 2, d.width(), 13}, bg);

      char rssi_str[8];
      std::snprintf(rssi_str, sizeof(rssi_str), "%d", ap.rssi);
      const char* lock = ap.secured ? "*" : " ";
      char line[40];
      std::snprintf(line, sizeof(line), "%s%-20s %s", lock, ap.ssid, rssi_str);
      d.draw_text(4, y + 2, line, fg, bg);
    }
    d.draw_text(8, d.height() - 14, "Enter:join Tab:scan Fn+Bk:forget",
                kJapanRedDark, kWhite);
  }

  void render_pass_(IDisplay& d) {
    char line[64];
    std::snprintf(line, sizeof(line), "SSID: %.32s", sel_ssid_);
    d.draw_text(8, 22, line, kBlack, kWhite);
    d.draw_text(8, 42, "Passphrase:", kJapanRedDark, kWhite);
    // Mask all but the most recent character so the user can confirm typing.
    char masked[kMaxPass + 1];
    if (pass_len_ == 0) {
      masked[0] = 0;
    } else {
      for (size_t i = 0; i + 1 < pass_len_; ++i) masked[i] = '*';
      masked[pass_len_ - 1] = pass_[pass_len_ - 1];
      masked[pass_len_]     = 0;
    }
    d.draw_text(8, 60, masked, kJapanRed, kWhite);
    d.draw_text(8, d.height() - 14, "Enter:join Esc:back",
                kJapanRedDark, kWhite);
  }

  void render_connecting_(IDisplay& d) {
    char line[64];
    std::snprintf(line, sizeof(line), "Joining %.20s ...", sel_ssid_);
    d.draw_text(8, 50, line, kJapanRed, kWhite);
  }

  void render_connected_(IDisplay& d) {
    char line[64];
    std::snprintf(line, sizeof(line), "Connected: %.20s", sel_ssid_);
    d.draw_text(8, 40, line, kJapanRed, kWhite);
    char ip_line[40];
    std::snprintf(ip_line, sizeof(ip_line), "IP %.16s", net_.wifi_ip());
    d.draw_text(8, 60, ip_line, kBlack, kWhite);
    d.draw_text(8, d.height() - 14, "Enter:list Fn+Bk:forget",
                kJapanRedDark, kWhite);
  }

  void render_failed_(IDisplay& d) {
    char line[64];
    std::snprintf(line, sizeof(line), "Failed: %.20s", sel_ssid_);
    d.draw_text(8, 50, line, kJapanRedBright, kWhite);
    d.draw_text(8, d.height() - 14, "Enter:retry", kJapanRedDark, kWhite);
  }

  INet&     net_;
  IStorage* store_      = nullptr;
  Menu      menu_;
  Hal*      hal_        = nullptr;
  State     state_      = State::Scanning;
  uint32_t  last_poll_  = 0;

  char     sel_ssid_[33] = {0};
  bool     sel_secured_  = false;
  char     pass_[kMaxPass] = {0};
  size_t   pass_len_    = 0;
};

}  // namespace yui

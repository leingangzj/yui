#pragma once
// PineappleReconApp — drive a recon scan on the Pineapple. v0.2 must-have
// scope: start scan, poll progress, show "complete" when done.
//
// State machine:
//   Idle      — Enter starts a scan
//   Starting  — POST /api/recon/start (one-shot)
//   Scanning  — poll /api/recon/status until scanRunning=false
//   Done      — show summary + "Enter to scan again"
//
// AP-list rendering is deferred to v0.3 (needs JsonValue array helper);
// for now Done just reports scan-id + estimated AP count from the
// dashboard.
#include "yui/app/App.hpp"
#include "yui/proto/Pineapple.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class PineappleReconApp : public App {
public:
  enum class State : uint8_t { Idle, Starting, Scanning, Done, Error };

  PineappleReconApp(IHttp& http, IStorage& store)
      : client_(http, &store), store_(store) {}

  const char* name() const override { return "Recon"; }
  Category    category() const override { return Category::WiFi; }

  void on_enter(Hal& hal) override {
    hal_           = &hal;
    state_         = State::Idle;
    scan_id_       = 0;
    percent_       = 0;
    last_poll_ms_  = 0;
    err_msg_[0]    = '\0';
  }

  void tick(uint32_t now_ms) override {
    if (state_ != State::Scanning) return;
    if (now_ms - last_poll_ms_ < kPollIntervalMs && last_poll_ms_ != 0) return;
    last_poll_ms_ = now_ms == 0 ? 1 : now_ms;
    int sid = 0; bool running = false; int pct = 0;
    if (!client_.recon_status(sid, running, pct)) {
      state_ = State::Error;
      std::strncpy(err_msg_, "status fetch failed", sizeof(err_msg_) - 1);
      return;
    }
    percent_ = pct;
    if (!running) state_ = State::Done;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key != Key::Enter) return;
    if (state_ == State::Idle || state_ == State::Done ||
        state_ == State::Error) {
      start_scan_();
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "Pineapple Recon");

    char line[40];
    int y = 30;
    switch (state_) {
      case State::Idle:
        d.draw_text(8, y, "Press Enter to scan", ui::kAccentDark, ui::kSurface);
        break;
      case State::Starting:
        d.draw_text(8, y, "Starting scan...", ui::kAccent, ui::kSurface);
        break;
      case State::Scanning: {
        std::snprintf(line, sizeof(line), "Scanning: %d%%", percent_);
        d.draw_text(8, y, line, ui::kAccent, ui::kSurface); y += 14;
        // Progress bar
        const int bar_w = 200;
        const int filled = (bar_w * percent_) / 100;
        d.fill_rect({8, y, bar_w, 8}, ui::kSurface);
        if (filled > 0) d.fill_rect({8, y, filled, 8}, ui::kAccent);
        break;
      }
      case State::Done:
        std::snprintf(line, sizeof(line), "Scan %d complete", scan_id_);
        d.draw_text(8, y, line, ui::kAccent, ui::kSurface); y += 14;
        d.draw_text(8, y, "(Results on Pineapple UI)",
                    ui::kAccentDark, ui::kSurface);
        break;
      case State::Error:
        d.draw_text(8, y, "Error:", kJapanRedBright, ui::kSurface); y += 14;
        d.draw_text(8, y, err_msg_, ui::kAccentDark, ui::kSurface);
        break;
    }
    d.draw_text(8, d.height() - 14,
                state_ == State::Idle ? "Enter:scan  Esc:back"
                : state_ == State::Scanning ? "Esc:back"
                : "Enter:rescan Esc:back",
                ui::kAccentDark, ui::kSurface);
    d.flush();
  }

  // Test hooks
  State    state()    const { return state_; }
  int      scan_id()  const { return scan_id_; }
  int      percent()  const { return percent_; }
  pineapple::Client& client() { return client_; }

private:
  static constexpr uint32_t kPollIntervalMs = 1000;

  void start_scan_() {
    state_         = State::Starting;
    last_poll_ms_  = 0;
    percent_       = 0;
    if (!ensure_authed_()) {
      state_ = State::Error;
      std::strncpy(err_msg_, "login failed", sizeof(err_msg_) - 1);
      return;
    }
    int sid = 0;
    if (!client_.recon_start(true, 30, "2.4ghz", sid)) {
      state_ = State::Error;
      std::strncpy(err_msg_, "scan start failed", sizeof(err_msg_) - 1);
      return;
    }
    scan_id_ = sid;
    state_   = State::Scanning;
  }

  bool ensure_authed_() {
    if (client_.authenticated()) return true;
    char host[64] = {0};
    if (!store_.get_str("pa.host", host, sizeof(host)) || host[0] == 0) return false;
    int32_t port = 1471;
    store_.get_int("pa.port", port, 1471);
    char user[40] = {0};
    char pass[40] = {0};
    store_.get_str("pa.user", user, sizeof(user));
    store_.get_str("pa.pass", pass, sizeof(pass));
    return client_.login(host, static_cast<uint16_t>(port), user, pass);
  }

  pineapple::Client client_;
  IStorage&         store_;
  Hal*              hal_           = nullptr;
  State             state_         = State::Idle;
  int               scan_id_       = 0;
  int               percent_       = 0;
  uint32_t          last_poll_ms_  = 0;
  char              err_msg_[32]   = {0};
};

}  // namespace yui

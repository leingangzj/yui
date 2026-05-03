#pragma once
// Clock app — three modes: stopwatch, countdown timer, time-of-day (NTP).
//   Tab    : cycle mode
//   Enter  : start/stop (stopwatch/timer); request NTP resync (TimeOfDay)
//   Bksp   : reset (stopwatch/timer)
//   Up/Dn  : (timer mode) adjust target ±10 s
#include "yui/app/App.hpp"
#include "yui/hal/INet.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <ctime>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#if !defined(YUI_TARGET_CARDPUTER_ADV)
#include <cstdlib>  // setenv on native so localtime_r honors TZ
#endif

namespace yui {

class ClockApp : public App {
public:
  enum class Mode { Stopwatch, Timer, TimeOfDay };

  // INet is optional — without it, TimeOfDay still renders the system clock
  // (which is 0/--:--:-- until something else syncs it), but Enter cannot
  // request a resync.
  ClockApp() = default;
  explicit ClockApp(INet* net,
                    const char* ntp_server = "pool.ntp.org",
                    const char* tz         = "UTC0",
                    IStorage* store        = nullptr)
      : net_(net), store_(store) {
    std::strncpy(ntp_buf_, ntp_server ? ntp_server : "pool.ntp.org",
                 sizeof(ntp_buf_) - 1);
    ntp_buf_[sizeof(ntp_buf_) - 1] = 0;
    std::strncpy(tz_buf_, tz ? tz : "UTC0", sizeof(tz_buf_) - 1);
    tz_buf_[sizeof(tz_buf_) - 1] = 0;
  }

  const char* name() const override { return "Clock"; }
  Category    category() const override { return Category::System; }

  void on_enter(Hal& hal) override {
    hal_     = &hal;
    mode_    = Mode::Stopwatch;
    running_ = false;
    elapsed_ = 0;
    target_  = 60'000;  // 1 min default for timer
    last_now_ = hal.clock.millis();
    // Pick up any TZ change made in Settings since the last open.
    if (store_) {
      char buf[40] = {0};
      if (store_->get_str("clock.tz", buf, sizeof(buf)) && buf[0] != 0) {
        std::strncpy(tz_buf_, buf, sizeof(tz_buf_) - 1);
        tz_buf_[sizeof(tz_buf_) - 1] = 0;
      }
      char nbuf[40] = {0};
      if (store_->get_str("clock.ntp", nbuf, sizeof(nbuf)) && nbuf[0] != 0) {
        std::strncpy(ntp_buf_, nbuf, sizeof(ntp_buf_) - 1);
        ntp_buf_[sizeof(ntp_buf_) - 1] = 0;
      }
    }
#if !defined(YUI_TARGET_CARDPUTER_ADV)
    ::setenv("TZ", tz_buf_, /*overwrite=*/1);
    ::tzset();
#endif
  }

  void tick(uint32_t now_ms) override {
    if (running_) {
      const uint32_t dt = now_ms - last_now_;
      elapsed_ += dt;
    }
    last_now_ = now_ms;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Tab) {
      mode_ = next_mode_(mode_);
      running_ = false;
      elapsed_ = 0;
      return;
    }
    if (mode_ == Mode::TimeOfDay) {
      if (k.key == Key::Enter && net_ && hal_) {
        // Best-effort: requires WiFi already up. Result shows next render.
        net_->ntp_sync(ntp_buf_, tz_buf_);
      }
      return;
    }
    if (k.key == Key::Enter)     { running_ = !running_; return; }
    if (k.key == Key::Backspace) { running_ = false; elapsed_ = 0; return; }
    if (mode_ == Mode::Timer) {
      if (k.key == Key::Up   && target_ + 10'000 <= 99 * 60'000) target_ += 10'000;
      if (k.key == Key::Down && target_ >= 10'000)               target_ -= 10'000;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    const char* title = "Stopwatch";
    if (mode_ == Mode::Timer)      title = "Timer";
    if (mode_ == Mode::TimeOfDay)  title = "Time";
    ui::Chrome::header(d, title);

    if (mode_ == Mode::TimeOfDay) {
      render_time_of_day_(d);
      d.flush();
      return;
    }

    uint32_t shown = 0;
    if (mode_ == Mode::Stopwatch) shown = elapsed_;
    else shown = (elapsed_ >= target_) ? 0 : (target_ - elapsed_);

    const uint32_t mm  = (shown / 60'000);
    const uint32_t ss  = (shown / 1000) % 60;
    const uint32_t ms  = (shown % 1000) / 10;

    char big[16];
    std::snprintf(big, sizeof(big), "%02u:%02u.%02u",
                  static_cast<unsigned>(mm), static_cast<unsigned>(ss),
                  static_cast<unsigned>(ms));
    d.draw_text(40, 50, big, kJapanRed, kWhite);

    if (mode_ == Mode::Timer && elapsed_ >= target_) {
      d.draw_text(80, 80, "DONE", kJapanRedBright, kWhite);
    }

    d.draw_text(8, d.height() - 14, running_ ? "Enter:stop  Tab:mode"
                                              : "Enter:start Tab:mode",
                kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  Mode     mode()    const { return mode_; }
  bool     running() const { return running_; }
  uint32_t elapsed() const { return elapsed_; }
  uint32_t target()  const { return target_; }

private:
  static Mode next_mode_(Mode m) {
    switch (m) {
      case Mode::Stopwatch: return Mode::Timer;
      case Mode::Timer:     return Mode::TimeOfDay;
      case Mode::TimeOfDay: return Mode::Stopwatch;
    }
    return Mode::Stopwatch;
  }

  void render_time_of_day_(IDisplay& d) {
    const uint64_t epoch = hal_ ? hal_->clock.epoch_seconds() : 0;
    if (epoch == 0) {
      d.draw_text(20, 50, "--:--:--", kJapanRed, kWhite);
      d.draw_text(8, 80, "no NTP sync", kJapanRedDark, kWhite);
      const char* hint = net_ ? "Enter:sync  Tab:mode" : "Tab:mode";
      d.draw_text(8, d.height() - 14, hint, kJapanRedDark, kWhite);
      return;
    }
    const std::time_t t = static_cast<std::time_t>(epoch);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char hms[16];
    std::snprintf(hms, sizeof(hms), "%02d:%02d:%02d",
                  tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
    d.draw_text(40, 50, hms, kJapanRed, kWhite);

    char ymd[16];
    std::snprintf(ymd, sizeof(ymd), "%04d-%02d-%02d",
                  tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday);
    d.draw_text(60, 80, ymd, kJapanRedDark, kWhite);
    d.draw_text(8, d.height() - 14,
                net_ ? "Enter:sync  Tab:mode" : "Tab:mode",
                kJapanRedDark, kWhite);
  }

  Hal*        hal_        = nullptr;
  INet*       net_        = nullptr;
  IStorage*   store_      = nullptr;
  char        ntp_buf_[40] = "pool.ntp.org";
  char        tz_buf_[40]  = "UTC0";
  Mode        mode_       = Mode::Stopwatch;
  bool        running_    = false;
  uint32_t    elapsed_    = 0;
  uint32_t    target_     = 60'000;
  uint32_t    last_now_   = 0;
};

}  // namespace yui

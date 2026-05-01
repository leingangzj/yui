#pragma once
// Clock app — split into stopwatch and countdown timer modes.
//   Tab    : switch mode
//   Enter  : start/stop
//   Backsp : reset
//   Up/Dn  : (timer mode) adjust target ±10 s
#include "yui/app/App.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <algorithm>

namespace yui {

class ClockApp : public App {
public:
  enum class Mode { Stopwatch, Timer };

  const char* name() const override { return "Clock"; }

  void on_enter(Hal& hal) override {
    hal_     = &hal;
    mode_    = Mode::Stopwatch;
    running_ = false;
    elapsed_ = 0;
    target_  = 60'000;  // 1 min default for timer
    last_now_ = hal.clock.millis();
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
      mode_    = (mode_ == Mode::Stopwatch) ? Mode::Timer : Mode::Stopwatch;
      running_ = false;
      elapsed_ = 0;
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
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, mode_ == Mode::Stopwatch ? "Stopwatch" : "Timer", kWhite, kJapanRed);

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
  Hal*     hal_      = nullptr;
  Mode     mode_     = Mode::Stopwatch;
  bool     running_  = false;
  uint32_t elapsed_  = 0;
  uint32_t target_   = 60'000;
  uint32_t last_now_ = 0;
};

}  // namespace yui

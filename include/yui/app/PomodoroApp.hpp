#pragma once
// Classic Pomodoro: 25 min work, 5 min break, tone on transition. Enter
// starts/pauses, Backspace resets, Tab cycles preset durations.
#include "yui/app/App.hpp"
#include "yui/hal/ISpeaker.hpp"
#include "yui/types.hpp"
#include <cstdint>
#include <cstdio>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class PomodoroApp : public App {
public:
  enum class Phase { Work, Break };
  enum class State { Idle, Running, Paused };

  struct Preset { uint32_t work_ms; uint32_t break_ms; const char* label; };
  static constexpr Preset kPresets[] = {
    {25u * 60u * 1000u, 5u  * 60u * 1000u, "25 / 5"},
    {50u * 60u * 1000u, 10u * 60u * 1000u, "50 / 10"},
    {15u * 60u * 1000u, 3u  * 60u * 1000u, "15 / 3"},
  };
  static constexpr size_t kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);

  explicit PomodoroApp(ISpeaker& spk) : spk_(spk) {}
  const char* name() const override { return "Pomodoro"; }
  Category    category() const override { return Category::Fun; }

  void on_enter(Hal& /*hal*/) override {
    spk_.init();
    state_     = State::Idle;
    phase_     = Phase::Work;
    preset_    = 0;
    last_ms_   = 0;
    have_last_ = false;
    elapsed_   = 0;
    completed_ = 0;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    switch (k.key) {
      case Key::Enter:
        if (state_ == State::Running) state_ = State::Paused;
        else { state_ = State::Running; }
        break;
      case Key::Backspace:
        state_   = State::Idle;
        phase_   = Phase::Work;
        elapsed_ = 0;
        break;
      case Key::Tab:
        // Cycle preset only while idle, otherwise it would warp the timer.
        if (state_ == State::Idle) preset_ = (preset_ + 1) % kPresetCount;
        break;
      default: break;
    }
  }

  void tick(uint32_t now_ms) override {
    const uint32_t dt = have_last_ ? (now_ms - last_ms_) : 0;
    last_ms_   = now_ms;
    have_last_ = true;
    if (state_ != State::Running) return;
    elapsed_ += dt;
    const uint32_t target = phase_target_();
    if (elapsed_ >= target) {
      // Transition.
      if (phase_ == Phase::Work) {
        ++completed_;
        spk_.tone(880, 200);
        phase_ = Phase::Break;
      } else {
        spk_.tone(523, 200);
        phase_ = Phase::Work;
      }
      elapsed_ = 0;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, phase_ == Phase::Work ? "Pomodoro WORK" : "Pomodoro BREAK");
    d.draw_text(d.width() - 60, 4, kPresets[preset_].label, kWhite, kJapanRed);

    const uint32_t remaining = (elapsed_ >= phase_target_()) ? 0
                                : (phase_target_() - elapsed_);
    const uint32_t mm = remaining / 60000u;
    const uint32_t ss = (remaining / 1000u) % 60u;
    char big[16];
    std::snprintf(big, sizeof(big), "%02u:%02u", mm, ss);
    d.draw_text(d.width() / 2 - 30, 40, big, kBlack, kWhite);

    char status[40];
    const char* st = state_ == State::Idle    ? "idle"
                   : state_ == State::Running ? "running"
                                              : "paused";
    std::snprintf(status, sizeof(status), "%s  done %u", st, completed_);
    d.draw_text(8, d.height() - 28, status, kJapanRedDark, kWhite);
    d.draw_text(8, d.height() - 14, "Ent=start/pause Bksp=reset Tab=preset",
                kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  Phase    phase()     const { return phase_; }
  State    state()     const { return state_; }
  uint32_t elapsed_ms() const { return elapsed_; }
  size_t   preset()    const { return preset_; }
  unsigned completed() const { return completed_; }

private:
  uint32_t phase_target_() const {
    return phase_ == Phase::Work ? kPresets[preset_].work_ms
                                  : kPresets[preset_].break_ms;
  }

  ISpeaker& spk_;
  State    state_     = State::Idle;
  Phase    phase_     = Phase::Work;
  size_t   preset_    = 0;
  uint32_t last_ms_   = 0;
  bool     have_last_ = false;
  uint32_t elapsed_   = 0;
  unsigned completed_ = 0;
};

}  // namespace yui

#pragma once
// RfChaosApp — lab-only RF stress test. When armed, picks a random
// target from the active set every kIntensityIntervalMs and fires it
// briefly; logs (ms, target, channel) to SD as CSV. Refuses to arm
// unless FaradayMode is on.
//
// Targets are abstracted via `Target` so the app doesn't depend on
// concrete jammer/deauth/spam apps — main.cpp wires up the lambdas
// for whichever RF apps the build includes. Tests use stub targets
// to assert scheduling and log shape without touching any real radio.
#include "yui/app/App.hpp"
#include "yui/hal/IClock.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include "yui/util/FaradayMode.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>

namespace yui {

class RfChaosApp : public App {
 public:
  static constexpr int kMaxTargets = 8;

  struct Target {
    const char*           name;       // short label, e.g. "deauth"
    std::function<void()> fire;       // one-shot trigger (no-op safe)
    int                   default_ch; // optional, for logging
  };

  enum class State : uint8_t { Idle, Refused, Armed, Done };
  enum class Duration : uint8_t { OneMin = 0, FiveMin, FifteenMin, SixtyMin };

  RfChaosApp(IFs& fs, IClock& clock) : fs_(fs), clock_(clock) {}

  const char* name() const override { return "RF Chaos"; }
  Category    category() const override { return Category::Radio; }

  // Wire targets at startup. Call before on_enter or any time the
  // composition changes.
  void clear_targets() { target_count_ = 0; }
  bool add_target(const Target& t) {
    if (target_count_ >= kMaxTargets) return false;
    targets_[target_count_++] = t;
    return true;
  }

  void on_enter(Hal& /*hal*/) override {
    state_       = State::Idle;
    log_buf_off_ = 0;
    fired_count_ = 0;
    next_fire_ms_ = 0;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (state_ == State::Idle) {
      if (k.key == Key::Up   && duration_ != Duration::OneMin)
        duration_ = static_cast<Duration>(static_cast<int>(duration_) - 1);
      if (k.key == Key::Down && duration_ != Duration::SixtyMin)
        duration_ = static_cast<Duration>(static_cast<int>(duration_) + 1);
      if (k.key == Key::Left  && intensity_ > 1)  --intensity_;
      if (k.key == Key::Right && intensity_ < 10) ++intensity_;
      if (k.key == Key::Enter && k.fn) try_arm_();
    } else if (state_ == State::Armed) {
      if (k.key == Key::Backspace) stop_(false);
    } else if (state_ == State::Refused || state_ == State::Done) {
      if (k.key == Key::Enter || k.key == Key::Esc) state_ = State::Idle;
    }
  }

  void tick(uint32_t now_ms) override {
    if (state_ != State::Armed) return;
    if (now_ms >= deadline_ms_) { stop_(true); return; }
    if (now_ms < next_fire_ms_) return;
    fire_random_(now_ms);
    next_fire_ms_ = now_ms + interval_ms_();
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "RF Chaos (LAB)");
    char line[40];
    int y = 22;
    std::snprintf(line, sizeof(line), "Duration : %d min", duration_minutes());
    d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 14;
    std::snprintf(line, sizeof(line), "Intensity: %d / 10", intensity_);
    d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 14;
    std::snprintf(line, sizeof(line), "Targets  : %d", target_count_);
    d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 14;
    std::snprintf(line, sizeof(line), "State    : %s",
                  state_ == State::Armed   ? "RUNNING"
                : state_ == State::Done    ? "done"
                : state_ == State::Refused ? "refused (no Faraday)"
                                            : "idle");
    d.draw_text(8, y, line,
                state_ == State::Armed ? ui::kWarn : ui::kOnSurface,
                ui::kSurface); y += 14;
    std::snprintf(line, sizeof(line), "Fired    : %u", static_cast<unsigned>(fired_count_));
    d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface);
    ui::Chrome::footer(d, "^/v:dur </>:int Fn+Enter:arm Bksp:stop");
    d.flush();
  }

  // Test hooks
  State    state()        const { return state_; }
  Duration duration()     const { return duration_; }
  void     set_duration(Duration d) { duration_ = d; }
  int      duration_minutes() const {
    switch (duration_) {
      case Duration::OneMin:     return 1;
      case Duration::FiveMin:    return 5;
      case Duration::FifteenMin: return 15;
      case Duration::SixtyMin:   return 60;
    }
    return 1;
  }
  int      intensity()    const { return intensity_; }
  void     set_intensity(int v) { intensity_ = (v < 1) ? 1 : (v > 10 ? 10 : v); }
  uint32_t fired_count()  const { return fired_count_; }
  int      target_count() const { return target_count_; }
  uint32_t deadline_ms()  const { return deadline_ms_; }
  // Deterministic seed for tests. Call before tick() to get reproducible firing.
  void     set_seed(uint32_t s) { rng_state_ = s ? s : 1; }

 private:
  uint32_t interval_ms_() const {
    // Higher intensity = shorter interval. Linear: 10→100ms, 1→1000ms.
    return static_cast<uint32_t>(1100 - 100 * intensity_);
  }

  void try_arm_() {
    if (!FaradayMode::is_active() || target_count_ == 0) {
      state_ = State::Refused;
      return;
    }
    state_ = State::Armed;
    fired_count_ = 0;
    log_buf_off_ = 0;
    const uint32_t now = clock_.millis();
    deadline_ms_  = now + static_cast<uint32_t>(duration_minutes()) * 60u * 1000u;
    next_fire_ms_ = now;
    if (rng_state_ == 0) rng_state_ = now ? now : 1;
    log_append_("# rfchaos session start\n");
  }

  void stop_(bool natural) {
    state_ = natural ? State::Done : State::Idle;
    flush_log_();
  }

  void fire_random_(uint32_t now_ms) {
    if (target_count_ == 0) return;
    const int idx = static_cast<int>(rng_() % static_cast<uint32_t>(target_count_));
    const Target& t = targets_[idx];
    if (t.fire) t.fire();
    ++fired_count_;
    char line[80];
    std::snprintf(line, sizeof(line), "%u,%s,%d\n",
                  static_cast<unsigned>(now_ms),
                  t.name ? t.name : "?",
                  t.default_ch);
    log_append_(line);
  }

  // xorshift32 — deterministic with a seed, fine for "pick a target".
  uint32_t rng_() {
    uint32_t x = rng_state_;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    rng_state_ = x ? x : 1;
    return x;
  }

  void log_append_(const char* line) {
    const std::size_t n = std::strlen(line);
    if (log_buf_off_ + n + 1 >= sizeof(log_buf_)) flush_log_();
    if (log_buf_off_ + n + 1 >= sizeof(log_buf_)) return;  // line too big
    std::memcpy(log_buf_ + log_buf_off_, line, n);
    log_buf_off_ += n;
    log_buf_[log_buf_off_] = '\0';
  }

  void flush_log_() {
    if (log_buf_off_ == 0) return;
    fs_.init();
    char path[40];
    std::snprintf(path, sizeof(path), "/rfchaos/%u.log",
                  static_cast<unsigned>(clock_.millis()));
    fs_.write_all(path, log_buf_, log_buf_off_);
    log_buf_off_ = 0;
  }

  IFs&     fs_;
  IClock&  clock_;
  State    state_     = State::Idle;
  Duration duration_  = Duration::OneMin;
  int      intensity_ = 5;
  Target   targets_[kMaxTargets];
  int      target_count_ = 0;
  uint32_t fired_count_  = 0;
  uint32_t deadline_ms_  = 0;
  uint32_t next_fire_ms_ = 0;
  uint32_t rng_state_    = 0;
  char     log_buf_[2048] = {0};
  std::size_t log_buf_off_ = 0;
};

}  // namespace yui

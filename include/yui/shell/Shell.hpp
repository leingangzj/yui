#pragma once
#include "yui/hal/Hal.hpp"
#include "yui/app/App.hpp"
#include "yui/app/Launcher.hpp"
#include "yui/types.hpp"
#include "yui/shell/Splash.hpp"

namespace yui {

// Shell phases: SPLASH → LAUNCHER → APP → LAUNCHER → ...
// The Shell owns the current state and feeds the right App each frame.
//
// HAL-only, no Arduino. Native tests drive it by injecting key events into the
// MockKeyboard and asserting on the NativeDisplay framebuffer.
class Shell {
public:
  enum class Phase { Splash, Launcher, App };

  Shell(Hal& hal, Launcher& launcher, const char* version,
        uint32_t splash_min_ms = 1500)
    : hal_(hal),
      launcher_(launcher),
      version_(version),
      splash_min_ms_(splash_min_ms) {}

  void start() {
    splash_start_ms_ = hal_.clock.millis();
    phase_           = Phase::Splash;
    launcher_.on_enter(hal_);
  }

  // Run one frame: pump input, advance state, render.
  void tick() {
    KeyEvent ev{};
    bool got_key = hal_.keyboard.poll(ev);
    const uint32_t now = hal_.clock.millis();

    switch (phase_) {
      case Phase::Splash: {
        const uint32_t elapsed = now - splash_start_ms_;
        const bool min_elapsed = elapsed >= splash_min_ms_;
        const bool key_pressed = got_key && ev.down;
        if (min_elapsed && key_pressed) {
          enter_launcher_();
        } else {
          render_splash(hal_.display, version_, elapsed);
        }
        break;
      }

      case Phase::Launcher: {
        if (got_key) launcher_.on_key(ev);
        if (App* a = launcher_.take_pending_launch()) {
          enter_app_(a);
        } else {
          launcher_.tick(now);
          launcher_.render(hal_.display);
        }
        break;
      }

      case Phase::App: {
        if (got_key && ev.down && ev.key == Key::Esc) {
          exit_app_();
        } else {
          if (got_key) current_app_->on_key(ev);
          current_app_->tick(now);
          current_app_->render(hal_.display);
        }
        break;
      }
    }
  }

  Phase phase() const { return phase_; }
  App*  current_app() const { return current_app_; }

private:
  void enter_launcher_() {
    phase_ = Phase::Launcher;
    launcher_.on_enter(hal_);
  }
  void enter_app_(App* a) {
    current_app_ = a;
    phase_       = Phase::App;
    current_app_->on_enter(hal_);
  }
  void exit_app_() {
    if (current_app_) current_app_->on_exit();
    current_app_ = nullptr;
    enter_launcher_();
  }

  Hal&        hal_;
  Launcher&   launcher_;
  const char* version_;
  uint32_t    splash_min_ms_;
  uint32_t    splash_start_ms_ = 0;
  Phase       phase_           = Phase::Splash;
  App*        current_app_     = nullptr;
};

}  // namespace yui

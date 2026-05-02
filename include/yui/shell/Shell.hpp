#pragma once
#include "yui/hal/Hal.hpp"
#include "yui/hal/IRemote.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/app/App.hpp"
#include "yui/app/Launcher.hpp"
#include "yui/app/RemoteApp.hpp"
#include "yui/types.hpp"
#include "yui/shell/Splash.hpp"
#include <functional>

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

  // Optional bindings for the global Fn+V remote-toggle chord. Both must
  // be set for the chord to do anything; the Shell stays unaware of remote
  // state when they're nullptr (keeps native tests trivial).
  void bind_remote(IRemote* remote, IStorage* store) {
    remote_ = remote;
    store_  = store;
  }

  // Override the splash renderer. Default = the legacy Braille koi
  // (render_splash). Device builds swap in render_boot_animation so the
  // 24-frame PNG sequence runs at boot. Signature stays HAL-only so the
  // override is portable.
  using SplashRenderer = std::function<void(IDisplay&, uint32_t /*elapsed_ms*/)>;
  void set_splash_renderer(SplashRenderer fn) {
    splash_render_ = std::move(fn);
  }

  // Run one frame: pump input, advance state, render.
  void tick() {
    KeyEvent ev{};
    bool got_key = hal_.keyboard.poll(ev);
    const uint32_t now = hal_.clock.millis();

    // Global chord: Fn+V toggles the remote viewer + persists the new
    // state. Eaten before any app sees it so we don't get accidental 'v'
    // characters typed into Notes/etc.
    if (got_key && ev.down && ev.fn && (ev.ch == 'v' || ev.ch == 'V')) {
      toggle_remote_();
      return;
    }

    switch (phase_) {
      case Phase::Splash: {
        const uint32_t elapsed = now - splash_start_ms_;
        const bool min_elapsed = elapsed >= splash_min_ms_;
        const bool key_pressed = got_key && ev.down;
        if (min_elapsed && key_pressed) {
          enter_launcher_();
        } else if (splash_render_) {
          splash_render_(hal_.display, elapsed);
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

  // Test hook: returns whether the chord successfully toggled remote.
  bool last_chord_toggled() const { return last_chord_toggled_; }

private:
  void toggle_remote_() {
    last_chord_toggled_ = false;
    if (!remote_) return;
    const bool new_state = !remote_->running();
    if (new_state) remote_->start();
    else           remote_->stop();
    if (store_) store_->put_int(kStorageKeyDevRemote, new_state ? 1 : 0);
    last_chord_toggled_ = true;
  }

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
  IRemote*    remote_          = nullptr;
  IStorage*   store_           = nullptr;
  bool        last_chord_toggled_ = false;
  SplashRenderer splash_render_;
};

}  // namespace yui

#pragma once
// RollJamApp — chained jam → capture → replay attack against rolling-
// code remotes (KeeLoq, Hopping, etc).
//
// Sequence:
//   1. Jam target frequency narrowband while user presses remote
//      (their first signal is buried in noise, never reaches receiver).
//   2. Stop jam, capture next press the user makes (this becomes the
//      "saved" signal).
//   3. Now jam blocks button 2; we replay button 1 to unlock when the
//      user is gone.
//
// Phase 4 lands the state machine UI; transmit scheduling and tight
// jam-then-listen toggle land in 4.5.

#include "yui/app/App.hpp"
#include "yui/hal/ICc1101.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"

namespace yui {

class RollJamApp : public App {
public:
  enum class Stage {
    Idle,        // ready, awaiting Enter
    Jamming,     // jamming target freq while user presses
    Captured,    // first signal captured, jam continues blocking button 2
    Replayed,    // saved signal transmitted
    CapMissing,
  };

  explicit RollJamApp(ICc1101* radio) : radio_(radio) {}
  const char* name() const override { return "RollJam"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& /*hal*/) override {
    stage_ = (radio_ && radio_->is_present()) ? Stage::Idle : Stage::CapMissing;
  }
  void on_exit() override { if (radio_) radio_->set_carrier(false); }

  void on_key(KeyEvent k) override {
    if (!k.down || stage_ == Stage::CapMissing) return;
    if (k.key == Key::Enter) {
      // Phase 4 stub: walk the state machine. Phase 4.5 wires the
      // actual capture + replay primitives.
      switch (stage_) {
        case Stage::Idle:     stage_ = Stage::Jamming;  break;
        case Stage::Jamming:  stage_ = Stage::Captured; break;
        case Stage::Captured: stage_ = Stage::Replayed; break;
        case Stage::Replayed: stage_ = Stage::Idle;     break;
        default: break;
      }
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (stage_ == Stage::CapMissing) {
      ui::Chrome::header(d, "RollJam", "no cap");
      ui::Chrome::dialog(d, "Hydra not found",
                         "CC1101 did not respond. Re-seat the cap.",
                         "OK", nullptr, true);
      d.flush();
      return;
    }
    const char* label = "Idle";
    switch (stage_) {
      case Stage::Idle:     label = "Idle";        break;
      case Stage::Jamming:  label = "JAMMING";     break;
      case Stage::Captured: label = "Captured";    break;
      case Stage::Replayed: label = "Replayed";    break;
      default: break;
    }
    ui::Chrome::header(d, "RollJam", label);
    ui::Chrome::stat(d, 0, "Stage", label);
    ui::Chrome::stat(d, 1, "Target", "433.92 MHz");
    d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 50,
                       "Press Enter to advance state.",
                       ui::kHint, ui::kSurface, FontStyle::Caption);
    d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 64,
                       "Capture+replay land in 4.5.",
                       ui::kHint, ui::kSurface, FontStyle::Caption);
    ui::Chrome::footer(d, "Enter:next  Esc:back");
    d.flush();
  }

  Stage stage() const { return stage_; }

private:
  ICc1101* radio_;
  Stage    stage_ = Stage::Idle;
};

}  // namespace yui

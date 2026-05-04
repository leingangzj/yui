#pragma once
// SubGhzReplayApp — transmit saved .sub files via the CC1101.
//
// Phase 4 lands the cap-detection scaffolding + the file picker UI;
// the actual edge-by-edge transmit (using transmitDirect + GDO0
// timing in tight loops) lands in Phase 4.5 once we've validated
// the timing accuracy on hardware.

#include "yui/app/App.hpp"
#include "yui/hal/ICc1101.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"

namespace yui {

class SubGhzReplayApp : public App {
public:
  SubGhzReplayApp(ICc1101* radio, IFs* fs) : radio_(radio), fs_(fs) {}
  const char* name() const override { return "Sub-GHz Replay"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& /*hal*/) override {
    cap_missing_ = (!radio_ || !radio_->is_present());
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (cap_missing_) {
      ui::Chrome::header(d, "Sub-GHz Replay", "no cap");
      ui::Chrome::dialog(d, "Hydra not found",
                         "CC1101 did not respond. Re-seat the cap.",
                         "OK", nullptr, true);
      d.flush();
      return;
    }
    ui::Chrome::header(d, "Sub-GHz Replay");
    ui::Chrome::stat(d, 0, "Path", "/sub/");
    ui::Chrome::stat(d, 1, "Files", "(picker WIP)");
    d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 50,
                       "Captures from SubGhzCapture",
                       ui::kHint, ui::kSurface, FontStyle::Caption);
    d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 64,
                       "land in /sub/. Replay arrives in 4.5.",
                       ui::kHint, ui::kSurface, FontStyle::Caption);
    ui::Chrome::footer(d, "Esc:back");
    d.flush();
  }

  bool cap_missing() const { return cap_missing_; }

private:
  ICc1101* radio_;
  IFs*     fs_;
  bool     cap_missing_ = false;
};

}  // namespace yui

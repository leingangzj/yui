#pragma once
// SubGhzBruteApp — fixed-code dictionary attack against sub-GHz
// remotes that don't use rolling codes (basic garage doors, cheap
// wireless socket switches, etc).
//
// Phase 4: scaffold + cap-detection. The de-Bruijn sequence builder
// and the dictionary-walk transmit loop arrive in 4.5.

#include "yui/app/App.hpp"
#include "yui/hal/ICc1101.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"

namespace yui {

class SubGhzBruteApp : public App {
public:
  explicit SubGhzBruteApp(ICc1101* radio) : radio_(radio) {}
  const char* name() const override { return "Sub-GHz Brute"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& /*hal*/) override {
    cap_missing_ = (!radio_ || !radio_->is_present());
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (cap_missing_) {
      ui::Chrome::header(d, "Sub-GHz Brute", "no cap");
      ui::Chrome::dialog(d, "Hydra not found",
                         "CC1101 did not respond. Re-seat the cap.",
                         "OK", nullptr, true);
      d.flush();
      return;
    }
    ui::Chrome::header(d, "Sub-GHz Brute");
    ui::Chrome::stat(d, 0, "Mode", "dictionary");
    ui::Chrome::stat(d, 1, "Targets", "fixed-code");
    d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 50,
                       "De Bruijn walker arrives in 4.5",
                       ui::kHint, ui::kSurface, FontStyle::Caption);
    ui::Chrome::footer(d, "Esc:back");
    d.flush();
  }

  bool cap_missing() const { return cap_missing_; }

private:
  ICc1101* radio_;
  bool     cap_missing_ = false;
};

}  // namespace yui

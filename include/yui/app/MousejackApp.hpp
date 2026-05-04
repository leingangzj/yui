#pragma once
// MousejackApp — Logitech Unifying / generic 2.4 GHz wireless HID
// keystroke injection (Bastille's Mousejack research).
//
// Phase 4 lands the cap-detect scaffolding + scan-for-targets state.
// The actual scan-for-MAC + injection-payload runner lands in 4.5
// once we've prototyped the timing on hardware.

#include "yui/app/App.hpp"
#include "yui/hal/INrf24.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"

namespace yui {

class MousejackApp : public App {
public:
  explicit MousejackApp(INrf24* radio) : radio_(radio) {}
  const char* name() const override { return "Mousejack"; }
  Category    category() const override { return Category::Bluetooth; }

  void on_enter(Hal& /*hal*/) override {
    cap_missing_ = (!radio_ || !radio_->is_present());
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (cap_missing_) {
      ui::Chrome::header(d, "Mousejack", "no cap");
      ui::Chrome::dialog(d, "Hydra not found",
                         "nRF24 did not respond. Re-seat the cap.",
                         "OK", nullptr, true);
      d.flush();
      return;
    }
    ui::Chrome::header(d, "Mousejack");
    ui::Chrome::stat(d, 0, "Targets", "0 found");
    ui::Chrome::stat(d, 1, "Status", "Phase 4 stub");
    d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 50,
                       "Logitech Unifying scan + inject",
                       ui::kHint, ui::kSurface, FontStyle::Caption);
    d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 64,
                       "lands in 4.5.",
                       ui::kHint, ui::kSurface, FontStyle::Caption);
    ui::Chrome::footer(d, "Esc:back");
    d.flush();
  }

  bool cap_missing() const { return cap_missing_; }

private:
  INrf24* radio_;
  bool    cap_missing_ = false;
};

}  // namespace yui

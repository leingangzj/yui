#pragma once
#include "yui/app/App.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"

namespace yui {

class AboutApp : public App {
public:
  explicit AboutApp(const char* version) : version_(version) {}

  const char* name() const override { return "About"; }
  Category    category() const override { return Category::System; }
  const assets::IconRef* icon() const override { return &assets::icons::kInfo(); }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "About");

    int y = ui::kBodyTopY;
    d.draw_text(ui::kBodyPadX, y, "Yui", ui::kAccent, ui::kSurface);
    y += ui::kBodyLineH;
    d.draw_text(ui::kBodyPadX, y, version_, ui::kAccentDark, ui::kSurface);
    y += ui::kBodyLineH + 6;
    d.draw_text(ui::kBodyPadX, y, "Cardputer ADV", ui::kOnSurface, ui::kSurface);
    y += ui::kBodyLineH;
    d.draw_text(ui::kBodyPadX, y, "MIT licensed", ui::kOnSurface, ui::kSurface);

    ui::Chrome::footer(d, "Esc:back");
    d.flush();
  }

private:
  const char* version_;
};

}  // namespace yui

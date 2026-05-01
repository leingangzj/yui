#pragma once
#include "yui/app/App.hpp"
#include "yui/types.hpp"

namespace yui {

class AboutApp : public App {
public:
  explicit AboutApp(const char* version) : version_(version) {}

  const char* name() const override { return "About"; }
  Category    category() const override { return Category::System; }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "About", kWhite, kJapanRed);

    d.draw_text(8,  26, "Yui",            kJapanRed,     kWhite);
    d.draw_text(8,  40, version_,         kJapanRedDark, kWhite);
    d.draw_text(8,  60, "Cardputer ADV",  kBlack,        kWhite);
    d.draw_text(8,  74, "MIT licensed",   kBlack,        kWhite);
    d.draw_text(8, 110, "Esc to go back", kJapanRedDark, kWhite);

    d.flush();
  }

private:
  const char* version_;
};

}  // namespace yui

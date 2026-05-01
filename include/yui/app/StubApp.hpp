#pragma once
#include "yui/app/App.hpp"
#include "yui/types.hpp"

namespace yui {

// Placeholder for apps that aren't built yet. Renders the app's intended name
// and a "coming soon" line so the launcher list is honest.
class StubApp : public App {
public:
  explicit StubApp(const char* n) : name_(n) {}

  const char* name() const override { return name_; }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, name_, kWhite, kJapanRed);

    d.draw_text(8, 40, "Coming soon",   kJapanRed,     kWhite);
    d.draw_text(8, 60, "Esc to go back", kJapanRedDark, kWhite);
    d.flush();
  }

private:
  const char* name_;
};

}  // namespace yui

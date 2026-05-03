#pragma once
// Monthly calendar view. Left/Right = ±1 day, Up/Down = ±7, Tab = next
// month, Backspace = previous month, Enter = jump to today.
#include "yui/app/App.hpp"
#include "yui/types.hpp"
#include "yui/util/Date.hpp"
#include <cstdio>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class CalendarApp : public App {
public:
  CalendarApp() : selected_{2026, 5, 1}, today_{2026, 5, 1} {}
  void set_today(date::Date t) { today_ = t; selected_ = t; }
  const char* name() const override { return "Calendar"; }
  Category    category() const override { return Category::Fun; }

  void on_enter(Hal& /*hal*/) override {
    selected_ = today_;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    switch (k.key) {
      case Key::Left:      selected_ = date::add_days(selected_, -1); break;
      case Key::Right:     selected_ = date::add_days(selected_,  1); break;
      case Key::Up:        selected_ = date::add_days(selected_, -7); break;
      case Key::Down:      selected_ = date::add_days(selected_,  7); break;
      case Key::Tab:       selected_ = date::add_months(selected_,  1); break;
      case Key::Backspace: selected_ = date::add_months(selected_, -1); break;
      case Key::Enter:     selected_ = today_; break;
      default: break;
    }
  }

  void render(IDisplay& d) override {
    static constexpr const char* kMonth[] = {
      "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
    };
    d.clear(ui::kSurface);
    char title[32];
    std::snprintf(title, sizeof(title), "%s %d",
                  kMonth[selected_.m - 1], selected_.y);
    ui::Chrome::header(d, title);

    // Day-of-week header.
    static constexpr const char* kDow[] = {"S","M","T","W","T","F","S"};
    const int colW = 30;
    const int gridX = 8;
    const int gridY = 22;
    for (int i = 0; i < 7; ++i)
      d.draw_text(gridX + i * colW + 10, gridY, kDow[i], ui::kAccentDark, ui::kSurface);

    // Walk the grid: row 0..5, col 0..6. First-of-month aligns to its DOW.
    const int first_dow = date::day_of_week(selected_.y, selected_.m, 1);
    const int dim       = date::days_in_month(selected_.y, selected_.m);
    for (int slot = 0; slot < 6 * 7; ++slot) {
      const int day = slot - first_dow + 1;
      if (day < 1 || day > dim) continue;
      const int row  = slot / 7;
      const int col  = slot % 7;
      const int x    = gridX + col * colW;
      const int y    = gridY + 14 + row * 14;
      const bool sel = (day == selected_.d);
      const bool today = (selected_.y == today_.y && selected_.m == today_.m && day == today_.d);
      const Color bg = sel ? ui::kAccent : ui::kSurface;
      const Color fg = sel ? ui::kSurface    : (today ? ui::kAccent : ui::kOnSurface);
      if (sel) d.fill_rect({x, y - 2, colW - 2, 12}, bg);
      char num[4];
      std::snprintf(num, sizeof(num), "%2d", day);
      d.draw_text(x + 8, y, num, fg, bg);
    }
    d.flush();
  }

  // Test hooks
  date::Date selected() const { return selected_; }
  date::Date today()    const { return today_; }

private:
  date::Date selected_;
  date::Date today_;
};

}  // namespace yui

#pragma once
// Two-level Flipper-Zero-style launcher: home screen lists categories
// (Radio/WiFi/Bluetooth/Tools/System/Fun) with hinomaru-red selection
// bar + status strip; Enter drills in to the apps in that category;
// Esc returns to the category list.
//
// The Shell still sees this as one App-like thing (via on_enter / on_key /
// tick / render / take_pending_launch). Drill-in is internal.
#include "yui/app/App.hpp"
#include "yui/app/AppRegistry.hpp"
#include "yui/shell/Menu.hpp"
#include "yui/sys/SysProbe.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <ctime>

namespace yui {

class Launcher : public App {
public:
  static constexpr int kHeaderH   = 16;
  static constexpr int kRowH      = 16;
  static constexpr int kRowPadX   = 8;
  static constexpr int kRowTextDy = 4;

  // Top-level category order (Flipper-style: Radio/WiFi/BT first as the
  // headline use cases, Tools/System after, Fun drawer last).
  static constexpr Category kCategories[] = {
    Category::Radio,
    Category::WiFi,
    Category::Bluetooth,
    Category::Tools,
    Category::System,
    Category::Fun,
  };
  static constexpr size_t kCategoryCount =
      sizeof(kCategories) / sizeof(kCategories[0]);

  explicit Launcher(AppRegistry& reg)
      : reg_(reg), cat_menu_(kCategoryCount), app_menu_(0) {}
  Launcher(AppRegistry& reg, SysProbe probe)
      : reg_(reg), cat_menu_(kCategoryCount), app_menu_(0),
        probe_(std::move(probe)), have_probe_(true) {}

  const char* name() const override { return "Yui"; }

  enum class View { Categories, Apps };

  void on_enter(Hal& /*hal*/) override {
    view_ = View::Categories;
    cat_menu_.set_count(kCategoryCount);
    refresh_app_menu_();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (view_ == View::Categories) {
      switch (k.key) {
        case Key::Up:    cat_menu_.up();   break;
        case Key::Down:  cat_menu_.down(); break;
        case Key::Enter:
          if (apps_in_(current_category_()) > 0) {
            view_ = View::Apps;
            refresh_app_menu_();
          }
          break;
        default: break;
      }
      return;
    }
    // View::Apps
    switch (k.key) {
      case Key::Up:    app_menu_.up();   break;
      case Key::Down:  app_menu_.down(); break;
      case Key::Esc:   view_ = View::Categories; break;
      case Key::Enter: pending_launch_ = selected_app_(); break;
      default: break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), kHeaderH}, kJapanRed);

    if (view_ == View::Categories) {
      d.draw_text(kRowPadX, 4, "Yui", kWhite, kJapanRed);
      render_categories_(d);
    } else {
      char title[24];
      std::snprintf(title, sizeof(title), "%s",
                    category_label(current_category_()));
      d.draw_text(kRowPadX, 4, title, kWhite, kJapanRed);
      render_apps_(d);
    }
    render_status_(d);
    d.flush();
  }

  // Test / shell hooks
  View         view()           const { return view_; }
  Category     current_category() const { return current_category_(); }
  std::size_t  cursor()         const {
    return view_ == View::Categories ? cat_menu_.cursor() : app_menu_.cursor();
  }
  App* selected() const { return selected_app_(); }
  App* take_pending_launch() {
    App* a = pending_launch_;
    pending_launch_ = nullptr;
    return a;
  }

  // Tests can drill in directly.
  void enter_category(Category c) {
    for (size_t i = 0; i < kCategoryCount; ++i) {
      if (kCategories[i] == c) { cat_menu_.set_cursor(i); break; }
    }
    if (apps_in_(c) > 0) {
      view_ = View::Apps;
      refresh_app_menu_();
    }
  }

private:
  Category current_category_() const { return kCategories[cat_menu_.cursor()]; }

  std::size_t apps_in_(Category c) const {
    std::size_t n = 0;
    for (std::size_t i = 0; i < reg_.size(); ++i) {
      App* a = reg_.at(i);
      if (a && a->category() == c) ++n;
    }
    return n;
  }

  App* nth_in_(Category c, std::size_t n) const {
    std::size_t k = 0;
    for (std::size_t i = 0; i < reg_.size(); ++i) {
      App* a = reg_.at(i);
      if (a && a->category() == c) {
        if (k == n) return a;
        ++k;
      }
    }
    return nullptr;
  }

  App* selected_app_() const {
    return nth_in_(current_category_(), app_menu_.cursor());
  }

  void refresh_app_menu_() {
    app_menu_.set_count(apps_in_(current_category_()));
  }

  void render_categories_(IDisplay& d) {
    char line[40];
    for (std::size_t i = 0; i < kCategoryCount; ++i) {
      const int y    = kHeaderH + 4 + static_cast<int>(i) * kRowH;
      const bool sel = (i == cat_menu_.cursor());
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : kJapanRed;
      if (sel) d.fill_rect({0, y - 2, d.width(), kRowH}, bg);
      const Category c = kCategories[i];
      std::snprintf(line, sizeof(line), "%-10s %u",
                    category_label(c),
                    static_cast<unsigned>(apps_in_(c)));
      d.draw_text(kRowPadX, y + kRowTextDy, line, fg, bg);
    }
  }

  void render_apps_(IDisplay& d) {
    const Category c = current_category_();
    const std::size_t n = apps_in_(c);
    if (n == 0) {
      d.draw_text(kRowPadX, kHeaderH + 24,
                  "(empty — coming soon)", kJapanRedDark, kWhite);
      d.draw_text(kRowPadX, d.height() - 14,
                  "Esc: back", kJapanRedDark, kWhite);
      return;
    }
    // Window the list to fit ~6 visible rows below the header.
    const std::size_t cur    = app_menu_.cursor();
    const std::size_t window = 6;
    const std::size_t start  = (cur >= window) ? (cur - window + 1) : 0;
    for (std::size_t i = start; i < n && i < start + window; ++i) {
      App* a = nth_in_(c, i);
      const int y    = kHeaderH + 4 + static_cast<int>(i - start) * kRowH;
      const bool sel = (i == cur);
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : kJapanRed;
      if (sel) d.fill_rect({0, y - 2, d.width(), kRowH}, bg);
      const char* label = a ? a->name() : "(null)";
      d.draw_text(kRowPadX, y + kRowTextDy, label, fg, bg);
    }
    d.draw_text(kRowPadX, d.height() - 14,
                "Enter:open  Esc:back", kJapanRedDark, kWhite);
  }

  void render_status_(IDisplay& d) {
    if (!have_probe_) return;
    char buf[32];
    int n = 0;
    if (probe_.battery_pct) {
      const int pct = probe_.battery_pct();
      if (pct >= 0)
        n += std::snprintf(buf + n, sizeof(buf) - n, "%d%% ", pct);
    }
    if (probe_.wifi_rssi) {
      const int rssi = probe_.wifi_rssi();
      if (rssi != 0)
        n += std::snprintf(buf + n, sizeof(buf) - n, "%d ", rssi);
    }
    bool clock_drawn = false;
    if (probe_.epoch_seconds) {
      const uint64_t e = probe_.epoch_seconds();
      if (e != 0) {
        const std::time_t t = static_cast<std::time_t>(e);
        std::tm tm_buf{};
#if defined(_WIN32)
        localtime_s(&tm_buf, &t);
#else
        localtime_r(&t, &tm_buf);
#endif
        n += std::snprintf(buf + n, sizeof(buf) - n, "%02d:%02d",
                           tm_buf.tm_hour, tm_buf.tm_min);
        clock_drawn = true;
      }
    }
    if (!clock_drawn && probe_.uptime_ms) {
      const uint32_t up = probe_.uptime_ms();
      const uint32_t hh = up / 3600000u;
      const uint32_t mm = (up / 60000u) % 60u;
      n += std::snprintf(buf + n, sizeof(buf) - n, "%02u:%02u", hh, mm);
    }
    if (n == 0) return;
    const int pixels = n * 6;
    const int x = d.width() - pixels - 4;
    d.draw_text(x, 4, buf, kWhite, kJapanRed);
  }

  AppRegistry& reg_;
  Menu cat_menu_;
  Menu app_menu_;
  View view_ = View::Categories;
  App* pending_launch_ = nullptr;
  SysProbe probe_{};
  bool have_probe_ = false;
};

}  // namespace yui

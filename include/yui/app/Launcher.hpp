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
#include <cstring>
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
      // Carousel: Left/Right flip pages, Up/Down kept as aliases for users
      // who reach for them out of habit (and for the existing test suite).
      switch (k.key) {
        case Key::Left:
        case Key::Up:    cat_menu_.up();   break;
        case Key::Right:
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

    if (view_ == View::Categories) {
      // No header bar in carousel mode — the page IS the category, full
      // bleed. Status strip floats top-right over the white background.
      render_carousel_(d);
    } else {
      d.fill_rect({0, 0, d.width(), kHeaderH}, kJapanRed);
      char title[24];
      std::snprintf(title, sizeof(title), "%s",
                    category_label(current_category_()));
      d.draw_text(kRowPadX, 4, title, kWhite, kJapanRed);
      render_apps_(d);
      render_status_(d);
    }
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

  // Fullscreen carousel: one category per page. Tile and label both
  // grew from the v0 layout — the previous 56-px tile + size-1 label
  // wasted half the panel, hard to read at arm's length.
  //
  //   ┌──────────────────────────┐  y=0
  //   │  status strip top-right  │
  //   │                          │  y=10
  //   │     ▓▓▓▓▓▓▓▓▓▓▓          │  y=12     icon tile (88×72)
  //   │     ▓ glyph    ▓          │
  //   │     ▓▓▓▓▓▓▓▓▓▓▓          │  y=84
  //   │                          │
  //   │      RADIO   (size 2)    │  y=92
  //   │      6 apps              │  y=110
  //   │      ● ○ ○ ○ ○ ○         │  y=128
  //   └──────────────────────────┘  y=135
  void render_carousel_(IDisplay& d) {
    const Category c = current_category_();
    const std::size_t n = apps_in_(c);
    const int W = d.width();
    const int H = d.height();

    constexpr int kTileW = 88;
    constexpr int kTileH = 72;
    const int tile_x = (W - kTileW) / 2;
    const int tile_y = 12;

    // Backplate: red rounded-ish square (chamfered with two interlocking
    // fill_rects so the corners read soft, not pixelated).
    d.fill_rect({tile_x + 3, tile_y,     kTileW - 6, kTileH},     kJapanRed);
    d.fill_rect({tile_x,     tile_y + 3, kTileW,     kTileH - 6}, kJapanRed);

    draw_category_glyph_(d, c, tile_x, tile_y, kTileW, kTileH);

    // Big category label (size 2). 12-px-wide glyphs.
    const char* label = category_label(c);
    const int   lw_2x = static_cast<int>(std::strlen(label)) * 12;
    d.draw_text_scaled((W - lw_2x) / 2, tile_y + kTileH + 4,
                       label, kJapanRed, kWhite, 2);

    // App count, size 1.
    char count_buf[20];
    if (n == 0) {
      std::snprintf(count_buf, sizeof(count_buf), "(empty)");
    } else if (n == 1) {
      std::snprintf(count_buf, sizeof(count_buf), "1 app");
    } else {
      std::snprintf(count_buf, sizeof(count_buf), "%u apps",
                    static_cast<unsigned>(n));
    }
    const int cw = label_pixel_width_(count_buf);
    d.draw_text((W - cw) / 2, tile_y + kTileH + 22, count_buf,
                kJapanRedDark, kWhite);

    // Dot pager — tucked at the bottom. Bigger filled dot, smaller dim
    // pips for the others.
    constexpr int kDotGap = 11;
    const int pager_w = static_cast<int>(kCategoryCount - 1) * kDotGap;
    const int pager_x = (W - pager_w) / 2;
    const int pager_y = H - 7;
    for (std::size_t i = 0; i < kCategoryCount; ++i) {
      const int cx = pager_x + static_cast<int>(i) * kDotGap;
      const bool sel = (i == cat_menu_.cursor());
      if (sel) d.fill_rect({cx - 4, pager_y - 2, 9, 5}, kJapanRed);
      else     d.fill_rect({cx - 1, pager_y,     3, 3}, kJapanRedDark);
    }

    render_status_floating_(d);
  }

  // Default M5GFX font is ~6 px wide per char with 1 px gap → 6 px total.
  static int label_pixel_width_(const char* s) {
    int n = 0;
    while (*s++) ++n;
    return n * 6;
  }

  // Per-category iconography painted from fill_rect primitives so we don't
  // need bitmaps or text scaling. Drawn inside a kTile×kTile red tile in
  // white; cx/cy is the tile's top-left corner. Glyphs scaled up for
  // the new 88×72 tile so they read at arm's length.
  void draw_category_glyph_(IDisplay& d, Category c, int cx, int cy,
                            int tw, int th) {
    const int mid_x = cx + tw / 2;
    const int mid_y = cy + th / 2;
    switch (c) {
      case Category::Radio: {
        // Antenna mast + 4 concentric "broadcast" arches.
        d.fill_rect({mid_x - 2, cy + 14, 4, th - 28}, kWhite);
        for (int i = 0; i < 4; ++i) {
          const int r = 12 + i * 8;
          d.fill_rect({mid_x - r, cy + 10 - i * 3, 2 * r, 3}, kWhite);
        }
        d.fill_rect({mid_x - 6, cy + th - 14, 13, 6}, kWhite);
        break;
      }
      case Category::WiFi: {
        // Three signal arcs of increasing width.
        for (int i = 0; i < 3; ++i) {
          const int w = 22 + i * 14;
          const int y = mid_y + 18 - i * 12;
          d.fill_rect({mid_x - w / 2, y, w, 4}, kWhite);
        }
        d.fill_rect({mid_x - 3, mid_y + 22, 7, 7}, kWhite);
        break;
      }
      case Category::Bluetooth: {
        // Stylized B-rune: vertical bar + crossed diagonals.
        d.fill_rect({mid_x - 2, cy + 12, 4, th - 24}, kWhite);
        for (int i = 0; i < 14; ++i) {
          d.fill_rect({mid_x + i, cy + 14 + i,           3, 3}, kWhite);
          d.fill_rect({mid_x + i, mid_y + 10 - i,        3, 3}, kWhite);
          d.fill_rect({mid_x + i, mid_y + i,             3, 3}, kWhite);
          d.fill_rect({mid_x + i, cy + th - 16 - i,      3, 3}, kWhite);
        }
        break;
      }
      case Category::Tools: {
        // Crossed wrench/screwdriver — axis-aligned stair-step diagonals.
        for (int i = 0; i < 22; ++i) {
          d.fill_rect({cx + 10 + i,         cy + 10 + i, 5, 5}, kWhite);
          d.fill_rect({cx + tw - 15 - i,    cy + 10 + i, 5, 5}, kWhite);
        }
        d.fill_rect({cx + 6,        cy + 6, 9, 9}, kWhite);
        d.fill_rect({cx + tw - 15,  cy + 6, 9, 9}, kWhite);
        break;
      }
      case Category::System: {
        // Gear: hub + 4 cardinal teeth + 4 corner teeth.
        d.fill_rect({mid_x - 12, mid_y - 12, 24, 24}, kWhite);
        d.fill_rect({mid_x - 4,  cy + 8,           8, 8}, kWhite);
        d.fill_rect({mid_x - 4,  cy + th - 16,     8, 8}, kWhite);
        d.fill_rect({cx + 8,     mid_y - 4,        8, 8}, kWhite);
        d.fill_rect({cx + tw - 16, mid_y - 4,      8, 8}, kWhite);
        d.fill_rect({mid_x - 4,  mid_y - 4, 8, 8}, kJapanRed);  // hub hole
        break;
      }
      case Category::Fun: {
        // Smiley: face + eyes + mouth, sized for the bigger tile.
        d.fill_rect({mid_x - 24, mid_y - 24, 48, 48}, kWhite);
        d.fill_rect({mid_x - 14, mid_y - 12, 6, 6}, kJapanRed);
        d.fill_rect({mid_x + 8,  mid_y - 12, 6, 6}, kJapanRed);
        d.fill_rect({mid_x - 12, mid_y + 8, 24, 4}, kJapanRed);
        d.fill_rect({mid_x - 14, mid_y + 6, 3, 4}, kJapanRed);
        d.fill_rect({mid_x + 11, mid_y + 6, 3, 4}, kJapanRed);
        break;
      }
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

  // Carousel mode: small dark-red status text in the top-right corner over
  // the white background (no header bar to invert against).
  void render_status_floating_(IDisplay& d) {
    char buf[32];
    const int n = format_status_(buf, sizeof(buf));
    if (n == 0) return;
    const int x = d.width() - n * 6 - 4;
    d.draw_text(x, 4, buf, kJapanRedDark, kWhite);
  }

  int format_status_(char* buf, std::size_t cap) {
    if (!have_probe_) return 0;
    int n = 0;
    if (probe_.battery_pct) {
      const int pct = probe_.battery_pct();
      if (pct >= 0)
        n += std::snprintf(buf + n, cap - static_cast<std::size_t>(n),
                           "%d%% ", pct);
    }
    if (probe_.wifi_rssi) {
      const int rssi = probe_.wifi_rssi();
      if (rssi != 0)
        n += std::snprintf(buf + n, cap - static_cast<std::size_t>(n),
                           "%d ", rssi);
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
        n += std::snprintf(buf + n, cap - static_cast<std::size_t>(n),
                           "%02d:%02d",
                           tm_buf.tm_hour, tm_buf.tm_min);
        clock_drawn = true;
      }
    }
    if (!clock_drawn && probe_.uptime_ms) {
      const uint32_t up = probe_.uptime_ms();
      const uint32_t hh = up / 3600000u;
      const uint32_t mm = (up / 60000u) % 60u;
      n += std::snprintf(buf + n, cap - static_cast<std::size_t>(n),
                         "%02u:%02u", hh, mm);
    }
    return n;
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

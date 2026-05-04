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
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>
#include <cstring>
#include <ctime>

namespace yui {

class Launcher : public App {
public:
  // Sized for the Phase-1 font system: Title-style header, Body-style
  // list rows. Local constants (independent of ui::kHeaderH) because the
  // Launcher's category-list view is a flat list, not the standard
  // Chrome layout.
  static constexpr int kHeaderH   = 24;   // matches ui::kHeaderH
  static constexpr int kRowH      = 24;   // Body-style row, comfortable
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
    d.clear(ui::kSurface);

    if (view_ == View::Categories) {
      // No header bar in carousel mode — the page IS the category, full
      // bleed. Status strip floats top-right over the surface.
      render_carousel_(d);
    } else {
      d.fill_rect({0, 0, d.width(), kHeaderH}, ui::kAccent);
      d.draw_text_styled(kRowPadX, 2, category_label(current_category_()),
                         ui::kOnAccent, ui::kAccent, FontStyle::Title);
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

  // Fullscreen carousel: one category per page.
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
    d.fill_rect({tile_x + 3, tile_y,     kTileW - 6, kTileH},     ui::kAccent);
    d.fill_rect({tile_x,     tile_y + 3, kTileW,     kTileH - 6}, ui::kAccent);

    draw_category_glyph_(d, c, tile_x, tile_y, kTileW, kTileH);

    // Big category label — Title style, centered under the tile.
    const char* label = category_label(c);
    const int   lw    = d.text_width(label, FontStyle::Title);
    d.draw_text_styled((W - lw) / 2, tile_y + kTileH + 4,
                       label, ui::kAccent, ui::kSurface, FontStyle::Title);

    // App count — Body style, dim, below the title.
    char count_buf[20];
    if (n == 0) {
      std::snprintf(count_buf, sizeof(count_buf), "(empty)");
    } else if (n == 1) {
      std::snprintf(count_buf, sizeof(count_buf), "1 app");
    } else {
      std::snprintf(count_buf, sizeof(count_buf), "%u apps",
                    static_cast<unsigned>(n));
    }
    const int cw = d.text_width(count_buf, FontStyle::Body);
    const int title_h = d.line_height(FontStyle::Title);
    d.draw_text_styled((W - cw) / 2, tile_y + kTileH + 4 + title_h + 2,
                       count_buf, ui::kHint, ui::kSurface, FontStyle::Body);

    // Dot pager — bottom of screen. Bigger filled dot, smaller dim
    // pips for the others.
    constexpr int kDotGap = 11;
    const int pager_w = static_cast<int>(kCategoryCount - 1) * kDotGap;
    const int pager_x = (W - pager_w) / 2;
    const int pager_y = H - 7;
    for (std::size_t i = 0; i < kCategoryCount; ++i) {
      const int cx = pager_x + static_cast<int>(i) * kDotGap;
      const bool sel = (i == cat_menu_.cursor());
      if (sel) d.fill_rect({cx - 4, pager_y - 2, 9, 5}, ui::kAccent);
      else     d.fill_rect({cx - 1, pager_y,     3, 3}, ui::kAccentDark);
    }

    render_status_floating_(d);
  }

  // Per-category iconography painted from fill_rect primitives so we don't
  // need bitmaps yet. Phase 2 swaps these for menu1/menu2 PNG icons via
  // d.draw_png; the same call site stays.
  void draw_category_glyph_(IDisplay& d, Category c, int cx, int cy,
                            int tw, int th) {
    const int mid_x = cx + tw / 2;
    const int mid_y = cy + th / 2;
    switch (c) {
      case Category::Radio: {
        // Antenna mast + 4 concentric "broadcast" arches.
        d.fill_rect({mid_x - 2, cy + 14, 4, th - 28}, ui::kSurface);
        for (int i = 0; i < 4; ++i) {
          const int r = 12 + i * 8;
          d.fill_rect({mid_x - r, cy + 10 - i * 3, 2 * r, 3}, ui::kSurface);
        }
        d.fill_rect({mid_x - 6, cy + th - 14, 13, 6}, ui::kSurface);
        break;
      }
      case Category::WiFi: {
        // Three signal arcs of increasing width.
        for (int i = 0; i < 3; ++i) {
          const int w = 22 + i * 14;
          const int y = mid_y + 18 - i * 12;
          d.fill_rect({mid_x - w / 2, y, w, 4}, ui::kSurface);
        }
        d.fill_rect({mid_x - 3, mid_y + 22, 7, 7}, ui::kSurface);
        break;
      }
      case Category::Bluetooth: {
        // Stylized B-rune: vertical bar + crossed diagonals.
        d.fill_rect({mid_x - 2, cy + 12, 4, th - 24}, ui::kSurface);
        for (int i = 0; i < 14; ++i) {
          d.fill_rect({mid_x + i, cy + 14 + i,           3, 3}, ui::kSurface);
          d.fill_rect({mid_x + i, mid_y + 10 - i,        3, 3}, ui::kSurface);
          d.fill_rect({mid_x + i, mid_y + i,             3, 3}, ui::kSurface);
          d.fill_rect({mid_x + i, cy + th - 16 - i,      3, 3}, ui::kSurface);
        }
        break;
      }
      case Category::Tools: {
        // Crossed wrench/screwdriver — axis-aligned stair-step diagonals.
        for (int i = 0; i < 22; ++i) {
          d.fill_rect({cx + 10 + i,         cy + 10 + i, 5, 5}, ui::kSurface);
          d.fill_rect({cx + tw - 15 - i,    cy + 10 + i, 5, 5}, ui::kSurface);
        }
        d.fill_rect({cx + 6,        cy + 6, 9, 9}, ui::kSurface);
        d.fill_rect({cx + tw - 15,  cy + 6, 9, 9}, ui::kSurface);
        break;
      }
      case Category::System: {
        // Gear: hub + 4 cardinal teeth + 4 corner teeth.
        d.fill_rect({mid_x - 12, mid_y - 12, 24, 24}, ui::kSurface);
        d.fill_rect({mid_x - 4,  cy + 8,           8, 8}, ui::kSurface);
        d.fill_rect({mid_x - 4,  cy + th - 16,     8, 8}, ui::kSurface);
        d.fill_rect({cx + 8,     mid_y - 4,        8, 8}, ui::kSurface);
        d.fill_rect({cx + tw - 16, mid_y - 4,      8, 8}, ui::kSurface);
        d.fill_rect({mid_x - 4,  mid_y - 4, 8, 8}, ui::kAccent);  // hub hole
        break;
      }
      case Category::Fun: {
        // Smiley: face + eyes + mouth, sized for the bigger tile.
        d.fill_rect({mid_x - 24, mid_y - 24, 48, 48}, ui::kSurface);
        d.fill_rect({mid_x - 14, mid_y - 12, 6, 6}, ui::kAccent);
        d.fill_rect({mid_x + 8,  mid_y - 12, 6, 6}, ui::kAccent);
        d.fill_rect({mid_x - 12, mid_y + 8, 24, 4}, ui::kAccent);
        d.fill_rect({mid_x - 14, mid_y + 6, 3, 4}, ui::kAccent);
        d.fill_rect({mid_x + 11, mid_y + 6, 3, 4}, ui::kAccent);
        break;
      }
    }
  }

  void render_apps_(IDisplay& d) {
    const Category c = current_category_();
    const std::size_t n = apps_in_(c);
    const int body_y = kHeaderH + 4;
    if (n == 0) {
      d.draw_text_styled(kRowPadX, body_y,
                         "(empty - coming soon)",
                         ui::kHint, ui::kSurface, FontStyle::Body);
      d.draw_text_styled(kRowPadX, d.height() - 12,
                         "Esc: back", ui::kHint, ui::kSurface,
                         FontStyle::Caption);
      return;
    }
    // Window the list to fit ~4-5 visible Body-sized rows below the
    // header (240×135 leaves ~107 px below the header for body content).
    const std::size_t cur    = app_menu_.cursor();
    const std::size_t window = 4;
    const std::size_t start  = (cur >= window) ? (cur - window + 1) : 0;
    for (std::size_t i = start; i < n && i < start + window; ++i) {
      App* a = nth_in_(c, i);
      const int y    = body_y + static_cast<int>(i - start) * kRowH;
      const bool sel = (i == cur);
      const Color bg = sel ? ui::kAccent : ui::kSurface;
      const Color fg = sel ? ui::kOnAccent : ui::kOnSurface;
      if (sel) d.fill_rect({0, y - 2, d.width(), kRowH}, bg);
      const char* label = a ? a->name() : "(null)";
      const int label_y = y + (kRowH - d.line_height(FontStyle::Body)) / 2;
      d.draw_text_styled(kRowPadX, label_y, label, fg, bg, FontStyle::Body);
    }

    // Right-edge scrollbar when the list overflows the window.
    if (n > window) {
      ui::Chrome::scrollbar(d, body_y, kRowH * static_cast<int>(window),
                            static_cast<int>(n),
                            static_cast<int>(window),
                            static_cast<int>(start));
    }

    d.draw_text_styled(kRowPadX, d.height() - 12,
                       "Enter:open  Esc:back",
                       ui::kHint, ui::kSurface, FontStyle::Caption);
  }

  // Carousel mode: small dim status text in the top-right corner over
  // the surface (no header bar to invert against).
  void render_status_floating_(IDisplay& d) {
    char buf[32];
    const int n = format_status_(buf, sizeof(buf));
    if (n == 0) return;
    const int w = d.text_width(buf, FontStyle::Caption);
    d.draw_text_styled(d.width() - w - 4, 4, buf,
                       ui::kHint, ui::kSurface, FontStyle::Caption);
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
    const int n = format_status_(buf, sizeof(buf));
    if (n == 0) return;
    const int w = d.text_width(buf, FontStyle::Caption);
    d.draw_text_styled(d.width() - w - 4, 6, buf,
                       ui::kOnAccent, ui::kAccent, FontStyle::Caption);
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

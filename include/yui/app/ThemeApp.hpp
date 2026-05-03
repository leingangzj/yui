#pragma once
// ThemeApp — pick a koi-themed palette for the whole OS.
//
// The selection is persisted to NVS under "ui.theme" (the palette's id).
// At boot, main.cpp reads the same key and calls ui::apply_palette() so
// the chosen theme survives reboots. Apps re-read ui::k* tokens every
// render so the swap is live — no need to rebuild any UI state.

#include "yui/app/App.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstring>

namespace yui {

inline constexpr const char* kStorageKeyTheme = "ui.theme";

class ThemeApp : public App {
public:
  explicit ThemeApp(IStorage* store) : store_(store) {}

  const char* name() const override { return "Theme"; }
  Category    category() const override { return Category::System; }

  void on_enter(Hal& /*hal*/) override {
    // Sync cursor to whatever palette is currently active so the user sees
    // their existing pick highlighted on entry.
    cursor_ = current_palette_index_();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    switch (k.key) {
      case Key::Up:
        if (cursor_ > 0) --cursor_;
        else cursor_ = ui::kPaletteCount - 1;
        break;
      case Key::Down:
        cursor_ = (cursor_ + 1) % ui::kPaletteCount;
        break;
      case Key::Enter:
        apply_and_persist_(cursor_);
        break;
      default: break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "Theme");

    for (std::size_t i = 0; i < ui::kPaletteCount; ++i) {
      const auto& p = ui::kPalettes[i];
      const bool sel = (i == cursor_);
      const int  y   = ui::kBodyTopY + static_cast<int>(i) * ui::kListRowH;

      // Selection bar
      const Color row_bg = sel ? ui::kAccent : ui::kSurface;
      const Color row_fg = sel ? ui::kOnAccent : ui::kOnSurface;
      if (sel) {
        d.fill_rect({0, y - 2, d.width(), ui::kListRowH}, row_bg);
      }

      // Color preview swatch — always shows the palette's accent / warn
      // colours regardless of selection state, so the user can compare
      // before committing.
      const int sx = d.width() - 60;
      d.fill_rect({sx,      y, 18, 10}, p.accent);
      d.fill_rect({sx + 22, y, 18, 10}, p.accent_dark);
      d.fill_rect({sx + 44, y, 18, 10}, p.warn);

      // Label + active marker
      const bool active = is_active_(p);
      char prefix = active ? '*' : ' ';
      char line[40];
      std::snprintf(line, sizeof(line), "%c %s", prefix, p.label);
      d.draw_text(ui::kBodyPadX, y + 4, line, row_fg, row_bg);
    }

    ui::Chrome::footer(d, "Enter:apply  Esc:back");
    d.flush();
  }

  // Test hooks
  std::size_t cursor() const { return cursor_; }

private:
  std::size_t current_palette_index_() const {
    if (!store_) return 0;
    char id[16] = {0};
    if (!store_->get_str(kStorageKeyTheme, id, sizeof(id))) return 0;
    for (std::size_t i = 0; i < ui::kPaletteCount; ++i) {
      if (std::strcmp(ui::kPalettes[i].id, id) == 0) return i;
    }
    return 0;
  }

  bool is_active_(const ui::Palette& p) const {
    // Cheap but correct: a palette is "active" iff its accent matches the
    // live ui::kAccent. (Two palettes never share a colour.)
    return p.accent == ui::kAccent;
  }

  void apply_and_persist_(std::size_t idx) {
    if (idx >= ui::kPaletteCount) return;
    const auto& p = ui::kPalettes[idx];
    ui::apply_palette(p);
    if (store_) store_->put_str(kStorageKeyTheme, p.id);
  }

  IStorage*   store_;
  std::size_t cursor_ = 0;
};

}  // namespace yui

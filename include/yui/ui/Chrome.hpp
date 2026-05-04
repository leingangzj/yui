#pragma once
// Chrome — the shared system-look every Yui app paints with.
//
// Use these helpers instead of hand-coding rectangles + text everywhere.
// One call per band, consistent geometry, single source of truth.
//
// Typical app render():
//
//   void render(IDisplay& d) override {
//     d.clear(ui::kSurface);
//     ui::Chrome::header(d, "My App", subtitle ? subtitle : nullptr);
//     // ... body, drawn against ui::kBodyTopY + n * ui::kBodyLineH ...
//     ui::Chrome::footer(d, "Enter:open  Esc:back");
//     d.flush();
//   }
//
// All text passes through IDisplay::draw_text_styled with a FontStyle
// from {Title, Body, Caption, Mono} — see hal/IDisplay.hpp.
//
// See docs/UI_CONVENTIONS.md for the full pattern.

#include "yui/hal/IDisplay.hpp"
#include "yui/types.hpp"
#include "yui/ui/Tokens.hpp"
#include "yui/assets/sprite_frame.hpp"

#include <cstdint>
#include <cstdio>

namespace yui::ui {

class Chrome {
public:
  // Top-of-screen accent strip. Title is rendered with FontStyle::Title
  // so the bar owns the page. Optional right-aligned subtitle (mode,
  // status, count) draws Caption-style.
  static void header(IDisplay& d, const char* title,
                     const char* subtitle = nullptr) {
    d.fill_rect({0, 0, d.width(), kHeaderH}, kAccent);
    d.draw_text_styled(kBodyPadX, 2, title, kOnAccent, kAccent,
                       FontStyle::Title);
    if (subtitle && *subtitle) {
      const int sw = d.text_width(subtitle, FontStyle::Caption);
      d.draw_text_styled(d.width() - sw - 4, 8, subtitle,
                         kOnAccent, kAccent, FontStyle::Caption);
    }
  }

  // Bottom hint line. Caption style, dim — reads as a hint, not a
  // heading. Pass nullptr to skip.
  static void footer(IDisplay& d, const char* hint) {
    if (!hint || !*hint) return;
    d.draw_text_styled(kBodyPadX, kFooterY, hint, kHint, kSurface,
                       FontStyle::Caption);
  }

  // Highlighted list-row at index `row` (0-based). Body-style label.
  // Optional left icon slot (24×24 PNG bytes); pass nullptr to skip and
  // the label hugs the left padding.
  static void list_row(IDisplay& d, int row, const char* label,
                       bool selected,
                       const uint8_t* icon_png = nullptr,
                       std::size_t icon_len = 0) {
    const int y_top  = kBodyTopY + row * kListRowH;
    const Color bg   = selected ? kAccent : kSurface;
    const Color fg   = selected ? kOnAccent : kOnSurface;
    if (selected) d.fill_rect({0, y_top - 2, d.width(), kListRowH}, bg);

    int label_x = kBodyPadX;
    if (icon_png && icon_len > 0) {
      const int icon_y = y_top + (kListRowH - 24) / 2 - 2;
      d.draw_png(icon_png, icon_len, kBodyPadX, icon_y);
      label_x = kBodyPadX + 24 + 6;  // icon + gap
    }

    // Center the body-text vertically inside the row.
    const int label_y = y_top + (kListRowH - d.line_height(FontStyle::Body)) / 2;
    d.draw_text_styled(label_x, label_y, label, fg, bg, FontStyle::Body);
  }

  // Small inline pill at (x,y). Caption style — compact "REC", "ON",
  // mode markers. Returns the pixel width consumed (incl. padding).
  static int badge(IDisplay& d, int x, int y, const char* text, bool on) {
    const int w = d.text_width(text, FontStyle::Caption) + 6;
    const Color bg = on ? kAccent : kSurface;
    const Color fg = on ? kOnAccent : kAccentDark;
    if (on) d.fill_rect({x, y - 2, w, d.line_height(FontStyle::Caption) + 4}, bg);
    d.draw_text_styled(x + 3, y, text, fg, bg, FontStyle::Caption);
    return w;
  }

  // Centered empty-state message in the body area. Optionally tiles a
  // small mascot sprite next to the message. Pass nullptr for sprite to
  // skip the koi (e.g. tests or non-thematic empty states).
  static void empty(IDisplay& d, const char* message,
                    const assets::SpriteFrame* mascot = nullptr,
                    int mascot_w = 0, int mascot_h = 0) {
    const int msg_w = d.text_width(message, FontStyle::Body);
    const int msg_h = d.line_height(FontStyle::Body);
    const int total_w = (mascot ? mascot_w + 8 : 0) + msg_w;
    const int x0 = (d.width() - total_w) / 2;
    const int y0 = (kBodyTopY + kFooterY) / 2 - msg_h / 2;

    int x = x0;
    if (mascot) {
      d.draw_png(mascot->data, mascot->len,
                 x, y0 - (mascot_h - msg_h) / 2);
      x += mascot_w + 8;
    }
    d.draw_text_styled(x, y0, message, kHint, kSurface, FontStyle::Body);
  }

  // Stat row: label on the left in Caption (dim), value on the right in
  // Body (bold-feeling). The most common app body shape.
  static void stat(IDisplay& d, int row, const char* label,
                   const char* value) {
    const int y = kBodyTopY + row * kBodyLineH;
    d.draw_text_styled(kBodyPadX, y + 4, label, kHint, kSurface,
                       FontStyle::Caption);
    d.draw_text_styled(kBodyPadX + 96, y, value, kOnSurface, kSurface,
                       FontStyle::Body);
  }

  // ── New widgets (Phase 1) ────────────────────────────────────────────

  // Slim segmented scrollbar on the right edge of the body area. Total
  // = total items, visible = how many fit on screen, top = first index
  // currently visible. Renders nothing when total <= visible.
  static void scrollbar(IDisplay& d, int top_y, int height,
                        int total, int visible, int top) {
    if (total <= visible || total <= 0) return;
    constexpr int W = 3;
    const int x = d.width() - W - 1;
    // Track
    d.fill_rect({x, top_y, W, height}, kAccentDark);
    // Thumb
    int thumb_h = (visible * height + total / 2) / total;
    if (thumb_h < 6) thumb_h = 6;
    if (thumb_h > height) thumb_h = height;
    int thumb_y = top_y + (top * (height - thumb_h) + (total - visible) / 2)
                  / (total - visible);
    if (thumb_y < top_y) thumb_y = top_y;
    if (thumb_y + thumb_h > top_y + height) thumb_y = top_y + height - thumb_h;
    d.fill_rect({x, thumb_y, W, thumb_h}, kAccent);
  }

  // Variable-item row: `Label:           < value >`. The classic
  // Flipper-style settings row — label on the left, current value on
  // the right between left/right arrows. `editable` highlights the
  // value box when the user is in change-value mode.
  static void var_item_row(IDisplay& d, int row,
                           const char* label, const char* value,
                           bool selected, bool editing) {
    const int y_top = kBodyTopY + row * kListRowH;
    const Color bg = selected ? kAccent : kSurface;
    const Color fg = selected ? kOnAccent : kOnSurface;
    if (selected) d.fill_rect({0, y_top - 2, d.width(), kListRowH}, bg);

    const int text_y = y_top + (kListRowH - d.line_height(FontStyle::Body)) / 2;
    d.draw_text_styled(kBodyPadX, text_y, label, fg, bg, FontStyle::Body);

    // Right-align the `< value >` group.
    const int val_w  = d.text_width(value, FontStyle::Body);
    const int arrow_w = d.text_width("< ", FontStyle::Body);
    const int group_w = arrow_w * 2 + val_w;
    const int gx = d.width() - kBodyPadX - group_w - 4;  // 4 px reserve for scrollbar

    const Color val_bg = editing ? kAccentDark : bg;
    const Color val_fg = editing ? kOnAccent : fg;
    if (editing) {
      d.fill_rect({gx - 2, y_top + 2, group_w + 4, kListRowH - 4}, val_bg);
    }
    d.draw_text_styled(gx,                       text_y, "<",   val_fg, val_bg, FontStyle::Body);
    d.draw_text_styled(gx + arrow_w,             text_y, value, val_fg, val_bg, FontStyle::Body);
    d.draw_text_styled(gx + arrow_w + val_w,     text_y, " >",  val_fg, val_bg, FontStyle::Body);
  }

  // Modal dialog box centered on the screen. Title in Title-style on an
  // accent bar, message in Body, two button labels at the bottom (the
  // selected one highlighted). Renders an opaque card so it can sit
  // over a dirty body. Pass nullptr for `secondary` to render a
  // single-button dialog.
  //
  // Geometry:
  //   - Card width  = display width - 2*margin
  //   - Card height = title_band + message_band + button_band
  //   - Centred vertically.
  static void dialog(IDisplay& d, const char* title, const char* message,
                     const char* primary, const char* secondary,
                     bool selected_is_primary) {
    constexpr int kMargin = 12;
    constexpr int kPad    = 6;
    constexpr int kTitleH = 22;
    constexpr int kButtonH = 18;

    const int card_w = d.width() - 2 * kMargin;
    const int msg_h_unit = d.line_height(FontStyle::Body);
    const int card_h = kTitleH + kPad + msg_h_unit * 2 + kPad + kButtonH + 2 * kPad;
    const int card_x = kMargin;
    const int card_y = (d.height() - card_h) / 2;

    // Card surface + border
    d.fill_rect({card_x - 2, card_y - 2, card_w + 4, card_h + 4}, kAccentDark);
    d.fill_rect({card_x, card_y, card_w, card_h}, kSurface);

    // Title band
    d.fill_rect({card_x, card_y, card_w, kTitleH}, kAccent);
    const int t_y = card_y + 2;
    d.draw_text_styled(card_x + kPad, t_y, title, kOnAccent, kAccent,
                       FontStyle::Title);

    // Message
    if (message && *message) {
      const int m_y = card_y + kTitleH + kPad;
      d.draw_text_styled(card_x + kPad, m_y, message,
                         kOnSurface, kSurface, FontStyle::Body);
    }

    // Buttons row (bottom)
    const int b_y = card_y + card_h - kButtonH - kPad;
    auto draw_btn = [&](int bx, int bw, const char* lbl, bool sel) {
      const Color bbg = sel ? kAccent : kSurface;
      const Color bfg = sel ? kOnAccent : kAccent;
      d.fill_rect({bx, b_y, bw, kButtonH}, bbg);
      d.fill_rect({bx, b_y, bw, 1}, kAccentDark);
      d.fill_rect({bx, b_y + kButtonH - 1, bw, 1}, kAccentDark);
      d.fill_rect({bx, b_y, 1, kButtonH}, kAccentDark);
      d.fill_rect({bx + bw - 1, b_y, 1, kButtonH}, kAccentDark);
      const int lw = d.text_width(lbl, FontStyle::Body);
      const int lh = d.line_height(FontStyle::Body);
      d.draw_text_styled(bx + (bw - lw) / 2,
                         b_y + (kButtonH - lh) / 2, lbl,
                         bfg, bbg, FontStyle::Body);
    };

    if (secondary && *secondary) {
      const int gap = 4;
      const int bw = (card_w - 2 * kPad - gap) / 2;
      draw_btn(card_x + kPad,                      bw, primary,  selected_is_primary);
      draw_btn(card_x + kPad + bw + gap,           bw, secondary, !selected_is_primary);
    } else {
      const int bw = card_w - 2 * kPad;
      draw_btn(card_x + kPad, bw, primary, true);
    }
  }

  // ── Cap-missing dialog (Hydra-aware shorthand) ───────────────────────
  // Specialized helper for hardware-cap apps. Paints a header that
  // signals the app the user just opened, then a single-button dialog
  // explaining that the chip didn't ACK on its CS pin. The chip and
  // CS strings flow into the message verbatim — callers pass things
  // like ("CC1101", "CS=13") for sub-GHz or ("nRF24", "CS=6").
  static void cap_missing_dialog(IDisplay& d, const char* app_title,
                                 const char* chip, const char* cs_label) {
    d.clear(kSurface);
    Chrome::header(d, app_title, "no cap");
    char body[80];
    std::snprintf(body, sizeof(body),
                  "%s did not respond on %s. Re-seat the cap.",
                  chip, cs_label);
    Chrome::dialog(d, "Hydra not found", body, "OK", nullptr, true);
    d.flush();
  }
};

}  // namespace yui::ui

#pragma once
// Pixel paint on a 60×24 grid (4×4 cells). Arrows move the cursor, Enter
// toggles the cell, Space sets the cell, Backspace clears the canvas,
// Tab saves to /yui-draw.pbm on SD. The file is loaded on entry.
#include "yui/app/App.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/types.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace yui {

class DrawApp : public App {
public:
  static constexpr int          kCols    = 60;
  static constexpr int          kRows    = 24;
  static constexpr int          kCellPx  = 4;
  static constexpr int          kBoardX  = 0;
  static constexpr int          kBoardY  = 18;
  static constexpr const char*  kPath    = "/yui-draw.pbm";

  explicit DrawApp(IFs& fs) : fs_(fs) {}
  const char* name() const override { return "Draw"; }

  void on_enter(Hal& /*hal*/) override {
    fs_.init();
    cx_ = kCols / 2;
    cy_ = kRows / 2;
    saved_flash_ = 0;
    clear_();
    load_();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    switch (k.key) {
      case Key::Up:    if (cy_ > 0) --cy_; break;
      case Key::Down:  if (cy_ < kRows - 1) ++cy_; break;
      case Key::Left:  if (cx_ > 0) --cx_; break;
      case Key::Right: if (cx_ < kCols - 1) ++cx_; break;
      case Key::Enter: toggle_at_(cx_, cy_); break;
      case Key::Space: set_at_(cx_, cy_, true); break;
      case Key::Backspace: clear_(); break;
      case Key::Tab:   save_(); break;
      default: break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    char title[40];
    std::snprintf(title, sizeof(title), "Draw  %d,%d", cx_, cy_);
    d.draw_text(8, 4, title, kWhite, kJapanRed);
    if (saved_flash_ > 0) d.draw_text(d.width() - 50, 4, "saved", kWhite, kJapanRed);

    d.fill_rect({kBoardX, kBoardY,
                 kCols * kCellPx, kRows * kCellPx}, kJapanRedDark);
    for (int y = 0; y < kRows; ++y)
      for (int x = 0; x < kCols; ++x)
        if (cells_[y][x])
          d.fill_rect({kBoardX + x * kCellPx,
                       kBoardY + y * kCellPx,
                       kCellPx, kCellPx}, kWhite);

    // Cursor outline (single-pixel hot border).
    d.fill_rect({kBoardX + cx_ * kCellPx, kBoardY + cy_ * kCellPx,
                 kCellPx, 1}, kJapanRedBright);
    d.fill_rect({kBoardX + cx_ * kCellPx, kBoardY + cy_ * kCellPx + kCellPx - 1,
                 kCellPx, 1}, kJapanRedBright);
    d.fill_rect({kBoardX + cx_ * kCellPx, kBoardY + cy_ * kCellPx,
                 1, kCellPx}, kJapanRedBright);
    d.fill_rect({kBoardX + cx_ * kCellPx + kCellPx - 1, kBoardY + cy_ * kCellPx,
                 1, kCellPx}, kJapanRedBright);

    d.draw_text(8, d.height() - 14, "Ent=tgl Tab=save Bksp=clr",
                kJapanRedDark, kWhite);
    d.flush();
  }

  void tick(uint32_t /*now_ms*/) override {
    if (saved_flash_ > 0) --saved_flash_;
  }

  // Test hooks
  bool at(int x, int y) const { return cells_[y][x] != 0; }
  int  cursor_x() const { return cx_; }
  int  cursor_y() const { return cy_; }
  bool just_saved() const { return saved_flash_ > 0; }

private:
  void clear_() { std::memset(cells_, 0, sizeof(cells_)); }
  void set_at_(int x, int y, bool v) {
    cells_[y][x] = v ? 1 : 0;
  }
  void toggle_at_(int x, int y) {
    cells_[y][x] = cells_[y][x] ? 0 : 1;
  }

  bool save_() {
    // PBM P1 ASCII: header "P1\n<W> <H>\n", then 0/1 separated by spaces.
    char buf[16 + kCols * kRows * 2 + 4];
    int n = std::snprintf(buf, sizeof(buf), "P1\n%d %d\n", kCols, kRows);
    for (int y = 0; y < kRows; ++y) {
      for (int x = 0; x < kCols; ++x) {
        buf[n++] = cells_[y][x] ? '1' : '0';
        buf[n++] = (x == kCols - 1) ? '\n' : ' ';
      }
    }
    if (!fs_.write_all(kPath, buf, static_cast<size_t>(n))) return false;
    saved_flash_ = 60;
    return true;
  }

  void load_() {
    if (!fs_.exists(kPath)) return;
    char buf[16 + kCols * kRows * 2 + 4];
    const int n = fs_.read_all(kPath, buf, sizeof(buf));
    if (n <= 0) return;
    // Skip "P1" header line and the dimensions line.
    int i = 0;
    int newlines = 0;
    while (i < n && newlines < 2) {
      if (buf[i] == '\n') ++newlines;
      ++i;
    }
    int x = 0, y = 0;
    while (i < n && y < kRows) {
      const char c = buf[i++];
      if (c == '0' || c == '1') {
        cells_[y][x] = (c == '1') ? 1 : 0;
        ++x;
        if (x >= kCols) { x = 0; ++y; }
      }
    }
  }

  IFs&    fs_;
  uint8_t cells_[kRows][kCols] = {};
  int     cx_ = 0;
  int     cy_ = 0;
  int     saved_flash_ = 0;
};

}  // namespace yui

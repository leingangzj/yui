#pragma once
// Tiny line-oriented notes editor. One file at /yui-notes.txt on SD. The
// buffer is kept short (capped at kBufCap) — this is a notes app, not a
// word processor. Insertion-point cursor with arrow-key nav, mid-buffer
// edits, and viewport scroll on both axes.
#include "yui/app/App.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/types.hpp"
#include <algorithm>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class NotesApp : public App {
public:
  static constexpr size_t      kBufCap       = 1024;
  static constexpr const char* kPath         = "/yui-notes.txt";
  static constexpr int         kVisibleLines = 7;
  static constexpr int         kVisibleCols  = 38;
  static constexpr int         kCharW        = 6;
  static constexpr int         kLineH        = 14;

  explicit NotesApp(IFs& fs) : fs_(fs) {}
  const char* name() const override { return "Notes"; }

  void on_enter(Hal& /*hal*/) override {
    fs_.init();
    len_ = 0;
    if (fs_.exists(kPath)) {
      const int n = fs_.read_all(kPath, buf_, kBufCap);
      if (n > 0) len_ = static_cast<size_t>(n);
    }
    buf_[len_]   = '\0';
    cur_         = len_;
    target_col_  = current_col_();
    dirty_       = false;
    saved_flash_ = 0;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    switch (k.key) {
      case Key::Backspace: backspace_(); return;
      case Key::Enter:     insert_('\n'); return;
      case Key::Space:     insert_(' '); return;
      case Key::Left:      if (k.fn) move_home_(); else move_left_();  return;
      case Key::Right:     if (k.fn) move_end_();  else move_right_(); return;
      case Key::Up:        move_up_(); return;
      case Key::Down:      move_down_(); return;
      case Key::Tab:       save_(); return;
      case Key::Char:
        if (k.ch >= 0x20 && k.ch < 0x7F) insert_(k.ch);
        return;
      default: return;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, dirty_ ? "Notes *" : "Notes", "Tab=save");

    const size_t cur_line = current_line_();
    const size_t cur_col  = current_col_();

    // Vertical scroll: keep cursor within window.
    if (cur_line < top_line_) top_line_ = cur_line;
    if (cur_line >= top_line_ + kVisibleLines)
      top_line_ = cur_line - kVisibleLines + 1;

    // Horizontal scroll: keep cursor's column within visible columns.
    if (cur_col < left_col_) left_col_ = cur_col;
    if (cur_col >= left_col_ + kVisibleCols)
      left_col_ = cur_col - kVisibleCols + 1;

    // Walk lines, render visible window.
    size_t line_idx = 0;
    size_t line_start = 0;
    for (size_t i = 0; i <= len_ && line_idx < top_line_ + kVisibleLines; ++i) {
      const bool eol = (i == len_) || (buf_[i] == '\n');
      if (eol) {
        if (line_idx >= top_line_) {
          char line[kVisibleCols + 1];
          const size_t line_len = i - line_start;
          const size_t off = std::min<size_t>(line_len, left_col_);
          const size_t take = std::min<size_t>(line_len - off, static_cast<size_t>(kVisibleCols));
          std::memcpy(line, buf_ + line_start + off, take);
          line[take] = '\0';
          const int y = 22 + static_cast<int>(line_idx - top_line_) * kLineH;
          d.draw_text(4, y, line, ui::kOnSurface, ui::kSurface);
        }
        line_idx++;
        line_start = i + 1;
      }
    }

    // Cursor: thin vertical bar at insertion point.
    if (cur_line >= top_line_ && cur_line < top_line_ + kVisibleLines &&
        cur_col >= left_col_ && cur_col < left_col_ + kVisibleCols) {
      const int cx = 4 + static_cast<int>(cur_col - left_col_) * kCharW;
      const int cy = 22 + static_cast<int>(cur_line - top_line_) * kLineH;
      d.fill_rect({cx, cy, 1, kLineH - 2}, ui::kAccent);
    }

    if (saved_flash_ > 0)
      d.draw_text(d.width() - 50, d.height() - 14, "saved", ui::kAccent, ui::kSurface);
    d.flush();
  }

  void tick(uint32_t /*now_ms*/) override {
    if (saved_flash_ > 0) --saved_flash_;
  }

  // Test hooks
  const char* buffer() const { return buf_; }
  size_t      buffer_len() const { return len_; }
  size_t      cursor() const { return cur_; }
  size_t      cursor_line() const { return current_line_(); }
  size_t      cursor_col() const { return current_col_(); }
  size_t      top_line() const { return top_line_; }
  size_t      left_col() const { return left_col_; }
  bool        dirty() const { return dirty_; }

private:
  size_t current_line_() const {
    size_t line = 0;
    for (size_t i = 0; i < cur_; ++i) if (buf_[i] == '\n') ++line;
    return line;
  }
  size_t current_col_() const {
    size_t col = 0;
    for (size_t i = cur_; i > 0; --i) {
      if (buf_[i - 1] == '\n') break;
      ++col;
    }
    return col;
  }
  size_t line_start_offset_(size_t line) const {
    if (line == 0) return 0;
    size_t l = 0;
    for (size_t i = 0; i < len_; ++i) {
      if (buf_[i] == '\n') {
        ++l;
        if (l == line) return i + 1;
      }
    }
    return len_;
  }
  size_t line_length_(size_t line_start) const {
    size_t i = line_start;
    while (i < len_ && buf_[i] != '\n') ++i;
    return i - line_start;
  }

  void insert_(char c) {
    if (len_ + 1 >= kBufCap) return;
    if (cur_ < len_) std::memmove(buf_ + cur_ + 1, buf_ + cur_, len_ - cur_);
    buf_[cur_] = c;
    ++len_;
    ++cur_;
    buf_[len_] = '\0';
    dirty_ = true;
    target_col_ = current_col_();
  }
  void backspace_() {
    if (cur_ == 0) return;
    if (cur_ < len_) std::memmove(buf_ + cur_ - 1, buf_ + cur_, len_ - cur_);
    --len_;
    --cur_;
    buf_[len_] = '\0';
    dirty_ = true;
    target_col_ = current_col_();
  }
  void move_home_() {
    const size_t line = current_line_();
    cur_ = line_start_offset_(line);
    target_col_ = 0;
  }
  void move_end_() {
    const size_t line = current_line_();
    const size_t s    = line_start_offset_(line);
    cur_ = s + line_length_(s);
    target_col_ = current_col_();
  }
  void move_left_() {
    if (cur_ > 0) --cur_;
    target_col_ = current_col_();
  }
  void move_right_() {
    if (cur_ < len_) ++cur_;
    target_col_ = current_col_();
  }
  void move_up_() {
    const size_t line = current_line_();
    if (line == 0) { cur_ = 0; target_col_ = 0; return; }
    const size_t prev_start = line_start_offset_(line - 1);
    const size_t prev_len   = line_length_(prev_start);
    cur_ = prev_start + std::min<size_t>(target_col_, prev_len);
  }
  void move_down_() {
    const size_t line = current_line_();
    const size_t cur_start = line_start_offset_(line);
    const size_t cur_len   = line_length_(cur_start);
    const size_t next_start_candidate = cur_start + cur_len + 1;
    if (next_start_candidate > len_) {
      cur_ = len_;
      target_col_ = current_col_();
      return;
    }
    const size_t next_len = line_length_(next_start_candidate);
    cur_ = next_start_candidate + std::min<size_t>(target_col_, next_len);
  }
  bool save_() {
    if (!fs_.write_all(kPath, buf_, len_)) return false;
    dirty_       = false;
    saved_flash_ = 60;
    return true;
  }

  IFs&   fs_;
  char   buf_[kBufCap] = {0};
  size_t len_          = 0;
  size_t cur_          = 0;
  size_t target_col_   = 0;
  size_t top_line_     = 0;
  size_t left_col_     = 0;
  bool   dirty_        = false;
  int    saved_flash_  = 0;
};

}  // namespace yui

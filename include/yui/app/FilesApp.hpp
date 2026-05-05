#pragma once
// microSD directory browser. Read-only. Up/Down navigate, Enter into dirs,
// ".." pops back up. Cursor position is restored on pop. Enter on a file
// opens an inline viewer (first kViewBytes); Backspace pops back to list.
#include "yui/app/App.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/shell/Menu.hpp"
#include "yui/types.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class FilesApp : public App {
public:
  static constexpr size_t kMaxEntries = 64;
  static constexpr size_t kMaxDepth   = 8;
  static constexpr size_t kViewBytes  = 1024;
  static constexpr int    kViewLines  = 8;
  static constexpr int    kViewCols   = 38;

  enum class Mode { List, View };

  explicit FilesApp(IFs& fs) : fs_(fs), menu_(0) {}
  const char* name() const override { return "Files"; }

  void on_enter(Hal& /*hal*/) override {
    fs_.init();
    std::strncpy(cwd_, "/", sizeof(cwd_) - 1);
    cwd_[sizeof(cwd_) - 1] = '\0';
    depth_       = 0;
    mode_        = Mode::List;
    view_top_    = 0;
    view_len_    = 0;
    view_path_[0] = '\0';
    refresh_();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (mode_ == Mode::View) {
      switch (k.key) {
        case Key::Backspace: mode_ = Mode::List; break;
        case Key::Tab:       hex_ = !hex_; view_top_ = 0; break;
        case Key::Up:        if (view_top_ > 0) --view_top_; break;
        case Key::Down:      ++view_top_; break;  // clamped by render
        default: break;
      }
      return;
    }
    if (count_ == 0) return;
    switch (k.key) {
      case Key::Up:    menu_.up();   break;
      case Key::Down:  menu_.down(); break;
      case Key::Enter: enter_();     break;
      default: break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    char title[40];
    if (mode_ == Mode::View) {
      std::snprintf(title, sizeof(title), "View %.34s", view_path_);
    } else {
      std::snprintf(title, sizeof(title), "Files %.33s", cwd_);
    }
    ui::Chrome::header(d, title);

    if (mode_ == Mode::View) {
      render_view_(d);
    ui::Chrome::footer(d, "Bksp=back  Tab=hex");
      d.flush();
      return;
    }

    if (count_ == 0) {
      d.draw_text(8, 40, "(empty)", ui::kAccentDark, ui::kSurface);
      d.flush();
      return;
    }

    const size_t cur = menu_.cursor();
    const size_t window = 7;
    const size_t start = (cur >= window) ? (cur - window + 1) : 0;
    for (size_t i = start; i < count_ && i < start + window; ++i) {
      const int y = 22 + static_cast<int>(i - start) * 13;
      const bool sel = (i == cur);
      const Color bg = sel ? ui::kAccent : ui::kSurface;
      const Color fg = sel ? ui::kSurface    : ui::kOnSurface;
      if (sel) d.fill_rect({0, y - 2, d.width(), 13}, bg);

      char line[40];
      if (entries_[i].is_dir) {
        std::snprintf(line, sizeof(line), "[D] %s", entries_[i].name);
      } else {
        std::snprintf(line, sizeof(line), "    %-22s %u", entries_[i].name, entries_[i].size);
      }
      d.draw_text(4, y + 2, line, fg, bg);
    }
    d.flush();
  }

  // Test hooks
  size_t count() const { return count_; }
  std::size_t cursor() const { return menu_.cursor(); }
  const char* cwd() const { return cwd_; }
  const FsEntry& at(size_t i) const { return entries_[i]; }
  size_t depth() const { return depth_; }
  Mode mode() const { return mode_; }
  bool hex() const { return hex_; }
  const char* view_path() const { return view_path_; }
  size_t view_len() const { return view_len_; }
  const char* view_buf() const { return view_buf_; }

private:
  bool at_root_() const { return cwd_[0] == '/' && cwd_[1] == '\0'; }

  void refresh_() {
    size_t off = 0;
    if (!at_root_()) {
      std::strncpy(entries_[0].name, "..", sizeof(entries_[0].name) - 1);
      entries_[0].name[sizeof(entries_[0].name) - 1] = '\0';
      entries_[0].is_dir = true;
      entries_[0].size   = 0;
      off = 1;
    }
    const int n = fs_.list(cwd_, entries_ + off, kMaxEntries - off);
    const size_t added = (n < 0) ? 0 : static_cast<size_t>(n);
    count_ = off + added;
    menu_.set_count(count_);
  }

  void enter_() {
    const auto& e = entries_[menu_.cursor()];
    if (!e.is_dir) { open_file_(e); return; }
    if (std::strcmp(e.name, "..") == 0) { pop_(); return; }
    if (depth_ < kMaxDepth) {
      saved_cursors_[depth_] = menu_.cursor();
      ++depth_;
    }
    const size_t cwd_len  = std::strlen(cwd_);
    const size_t name_len = std::strlen(e.name);
    const size_t need     = cwd_len + 1 + name_len + 1;  // sep + nul
    if (need > sizeof(cwd_)) return;  // path too long, ignore
    // Use snprintf instead of strncat to avoid -Wrestrict overlap
    // warnings — strncat's source/dest can alias when both point at
    // the same buffer slot.
    char joined[sizeof(cwd_) + 64];   // headroom for separator + child name
    if (at_root_()) {
      std::snprintf(joined, sizeof(joined), "/%s", e.name);
    } else {
      std::snprintf(joined, sizeof(joined), "%s/%s", cwd_, e.name);
    }
    std::strncpy(cwd_, joined, sizeof(cwd_) - 1);
    cwd_[sizeof(cwd_) - 1] = '\0';
    refresh_();
    menu_.set_cursor(0);
  }

  void pop_() {
    if (at_root_()) return;
    size_t n = std::strlen(cwd_);
    while (n > 1 && cwd_[n - 1] != '/') --n;
    if (n > 1) --n;  // strip trailing slash unless we'd erase root
    cwd_[n] = '\0';
    if (cwd_[0] == '\0') { cwd_[0] = '/'; cwd_[1] = '\0'; }
    refresh_();
    if (depth_ > 0) {
      --depth_;
      menu_.set_cursor(saved_cursors_[depth_]);
    } else {
      menu_.set_cursor(0);
    }
  }

  void open_file_(const FsEntry& e) {
    // Build absolute path.
    if (at_root_())
      std::snprintf(view_path_, sizeof(view_path_), "/%s", e.name);
    else
      std::snprintf(view_path_, sizeof(view_path_), "%s/%s", cwd_, e.name);
    const int n = fs_.read_all(view_path_, view_buf_, kViewBytes);
    view_len_ = (n < 0) ? 0 : static_cast<size_t>(n);
    view_top_ = 0;
    hex_      = false;
    mode_     = Mode::View;
  }

  void render_view_(IDisplay& d) {
    if (view_len_ == 0) {
      d.draw_text(8, 40, "(empty)", ui::kAccentDark, ui::kSurface);
      return;
    }
    if (hex_) { render_hex_(d); return; }
    // Walk lines (split on '\n', cap each line at kViewCols), render
    // [view_top_, view_top_ + kViewLines).
    size_t line_idx = 0, i = 0;
    while (i <= view_len_ && line_idx < view_top_ + kViewLines) {
      const size_t line_start = i;
      while (i < view_len_ && view_buf_[i] != '\n') ++i;
      if (line_idx >= view_top_) {
        char line[kViewCols + 1];
        const size_t line_len = i - line_start;
        const size_t take = std::min<size_t>(line_len, static_cast<size_t>(kViewCols));
        for (size_t j = 0; j < take; ++j) {
          const char c = view_buf_[line_start + j];
          line[j] = (c >= 0x20 && c < 0x7F) ? c : '.';
        }
        line[take] = '\0';
        const int y = 22 + static_cast<int>(line_idx - view_top_) * 12;
        d.draw_text(4, y, line, ui::kOnSurface, ui::kSurface);
      }
      ++line_idx;
      ++i;
    }
    // Clamp scroll: if we drew nothing visible, snap top back.
    if (line_idx <= view_top_ && view_top_ > 0) {
      view_top_ = (line_idx == 0) ? 0 : line_idx - 1;
    }
    d.draw_text(d.width() - 90, d.height() - 14, "Bksp=back Tab=hex", ui::kAccentDark, ui::kSurface);
  }

  void render_hex_(IDisplay& d) {
    constexpr int kBytesPerRow = 8;
    const size_t total_rows = (view_len_ + kBytesPerRow - 1) / kBytesPerRow;
    if (view_top_ >= total_rows) view_top_ = total_rows ? total_rows - 1 : 0;
    for (int row = 0; row < kViewLines; ++row) {
      const size_t r = view_top_ + row;
      if (r >= total_rows) break;
      const size_t off = r * kBytesPerRow;
      char line[40];
      int n = std::snprintf(line, sizeof(line), "%04zx ", off);
      for (int b = 0; b < kBytesPerRow; ++b) {
        if (off + b < view_len_) {
          n += std::snprintf(line + n, sizeof(line) - n, "%02x ",
                             static_cast<unsigned char>(view_buf_[off + b]));
        } else {
          n += std::snprintf(line + n, sizeof(line) - n, "   ");
        }
      }
      // ASCII gutter.
      n += std::snprintf(line + n, sizeof(line) - n, " ");
      for (int b = 0; b < kBytesPerRow && off + b < view_len_; ++b) {
        const char c = view_buf_[off + b];
        line[n++] = (c >= 0x20 && c < 0x7F) ? c : '.';
      }
      line[n] = '\0';
      const int y = 22 + row * 12;
      d.draw_text(4, y, line, ui::kOnSurface, ui::kSurface);
    }
    d.draw_text(d.width() - 95, d.height() - 14, "Bksp=back Tab=text", ui::kAccentDark, ui::kSurface);
  }

  IFs&    fs_;
  Menu    menu_;
  char    cwd_[128]                 = {0};
  FsEntry entries_[kMaxEntries]     = {};
  size_t  count_                    = 0;
  size_t  saved_cursors_[kMaxDepth] = {0};
  size_t  depth_                    = 0;
  Mode    mode_                     = Mode::List;
  char    view_path_[192]           = {0};
  char    view_buf_[kViewBytes + 1] = {0};
  size_t  view_len_                 = 0;
  size_t  view_top_                 = 0;
  bool    hex_                      = false;
};

}  // namespace yui

#pragma once
// Tiny persistent todo list. Up/Down navigate, Space toggles done,
// Backspace deletes the selected item, Tab adds a new item (then type
// + Enter to commit, Esc would cancel but Esc exits the app at the
// shell level, so use Backspace to abandon an in-progress add).
//
// On-disk format (one item per line on /yui-todo.txt):
//   "[ ] do the thing"
//   "[x] already done"
//
// Hard caps on item count and per-item length keep allocation static.
#include "yui/app/App.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/shell/Menu.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class TodoApp : public App {
public:
  static constexpr size_t kMaxItems   = 32;
  static constexpr size_t kItemMaxLen = 48;
  static constexpr const char* kPath  = "/yui-todo.txt";

  enum class Mode { List, Editing };

  explicit TodoApp(IFs& fs) : fs_(fs), menu_(0) {}
  const char* name() const override { return "Todo"; }
  Category    category() const override { return Category::Fun; }

  void on_enter(Hal& /*hal*/) override {
    fs_.init();
    count_ = 0;
    mode_  = Mode::List;
    edit_buf_[0] = '\0';
    edit_len_    = 0;
    load_();
    menu_.set_count(count_);
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (mode_ == Mode::Editing) { handle_edit_(k); return; }
    switch (k.key) {
      case Key::Up:        menu_.up();   break;
      case Key::Down:      menu_.down(); break;
      case Key::Space:     toggle_();    break;
      case Key::Backspace: remove_();    break;
      case Key::Tab:       start_add_(); break;
      default: break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    char title[40];
    std::snprintf(title, sizeof(title), "Todo (%u)", static_cast<unsigned>(count_));
    ui::Chrome::header(d, title);

    if (mode_ == Mode::Editing) {
      d.draw_text(8, 30, "new item:", ui::kAccentDark, ui::kSurface);
      char line[kItemMaxLen + 4];
      std::snprintf(line, sizeof(line), "> %s_", edit_buf_);
      d.draw_text(8, 50, line, ui::kOnSurface, ui::kSurface);
    ui::Chrome::footer(d, "Sp:done  Bksp:del  Tab:add  Esc:back");
      d.flush();
      return;
    }

    if (count_ == 0) {
      d.draw_text(8, 40, "(no items — Tab to add)", ui::kAccentDark, ui::kSurface);
      d.flush();
      return;
    }

    constexpr size_t window = 7;
    const size_t cur = menu_.cursor();
    const size_t start = (cur >= window) ? cur - window + 1 : 0;
    for (size_t i = start; i < count_ && i < start + window; ++i) {
      const int  y   = 22 + static_cast<int>(i - start) * 13;
      const bool sel = (i == cur);
      const Color bg = sel ? ui::kAccent : ui::kSurface;
      const Color fg = sel ? ui::kSurface    : (items_[i].done ? ui::kAccentDark : ui::kOnSurface);
      if (sel) d.fill_rect({0, y - 2, d.width(), 13}, bg);
      char line[kItemMaxLen + 8];
      std::snprintf(line, sizeof(line), "%s %s",
                    items_[i].done ? "[x]" : "[ ]",
                    items_[i].text);
      d.draw_text(4, y + 2, line, fg, bg);
    }
    d.flush();
  }

  // Test hooks
  size_t count() const { return count_; }
  bool   done(size_t i) const { return (i < count_) && items_[i].done; }
  const char* text(size_t i) const { return (i < count_) ? items_[i].text : ""; }
  Mode   mode() const { return mode_; }

private:
  struct Item { char text[kItemMaxLen]; bool done; };

  void handle_edit_(KeyEvent k) {
    if (k.key == Key::Enter) {
      if (edit_len_ > 0 && count_ < kMaxItems) {
        std::memcpy(items_[count_].text, edit_buf_, edit_len_ + 1);
        items_[count_].done = false;
        ++count_;
        menu_.set_count(count_);
        save_();
      }
      mode_ = Mode::List;
      return;
    }
    if (k.key == Key::Backspace) {
      if (edit_len_ > 0) {
        edit_buf_[--edit_len_] = '\0';
      } else {
        mode_ = Mode::List;
      }
      return;
    }
    char c = 0;
    if (k.key == Key::Char && k.ch >= 0x20 && k.ch < 0x7F) c = k.ch;
    else if (k.key == Key::Space) c = ' ';
    if (c == 0) return;
    if (edit_len_ + 1 >= kItemMaxLen) return;
    edit_buf_[edit_len_++] = c;
    edit_buf_[edit_len_]   = '\0';
  }

  void toggle_() {
    if (count_ == 0) return;
    items_[menu_.cursor()].done = !items_[menu_.cursor()].done;
    save_();
  }
  void remove_() {
    if (count_ == 0) return;
    const size_t i = menu_.cursor();
    for (size_t j = i; j + 1 < count_; ++j) items_[j] = items_[j + 1];
    --count_;
    menu_.set_count(count_);
    save_();
  }
  void start_add_() {
    if (count_ >= kMaxItems) return;
    edit_buf_[0] = '\0';
    edit_len_    = 0;
    mode_        = Mode::Editing;
  }

  void save_() {
    char buf[kMaxItems * (kItemMaxLen + 8)];
    int n = 0;
    for (size_t i = 0; i < count_; ++i) {
      n += std::snprintf(buf + n, sizeof(buf) - n, "[%c] %s\n",
                         items_[i].done ? 'x' : ' ', items_[i].text);
    }
    fs_.write_all(kPath, buf, static_cast<size_t>(n));
  }
  void load_() {
    if (!fs_.exists(kPath)) return;
    char buf[kMaxItems * (kItemMaxLen + 8)];
    const int n = fs_.read_all(kPath, buf, sizeof(buf));
    if (n <= 0) return;
    int i = 0;
    while (i < n && count_ < kMaxItems) {
      // Expect "[x] text\n" or "[ ] text\n".
      if (i + 4 > n) break;
      const bool done = (buf[i + 1] == 'x' || buf[i + 1] == 'X');
      i += 4;  // skip "[?] "
      size_t t = 0;
      while (i < n && buf[i] != '\n' && t + 1 < kItemMaxLen) {
        items_[count_].text[t++] = buf[i++];
      }
      items_[count_].text[t] = '\0';
      items_[count_].done = done;
      ++count_;
      while (i < n && buf[i] != '\n') ++i;
      if (i < n) ++i;  // skip newline
    }
  }

  IFs&    fs_;
  Menu    menu_;
  Item    items_[kMaxItems] = {};
  size_t  count_ = 0;
  Mode    mode_  = Mode::List;
  char    edit_buf_[kItemMaxLen] = {0};
  size_t  edit_len_ = 0;
};

}  // namespace yui

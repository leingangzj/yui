#pragma once
// microSD directory browser. Read-only. Up/Down navigate, Enter into dirs,
// ".." pops back up. Cursor position is restored on pop.
#include "yui/app/App.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/shell/Menu.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class FilesApp : public App {
public:
  static constexpr size_t kMaxEntries = 64;
  static constexpr size_t kMaxDepth   = 8;

  explicit FilesApp(IFs& fs) : fs_(fs), menu_(0) {}
  const char* name() const override { return "Files"; }

  void on_enter(Hal& /*hal*/) override {
    fs_.init();
    std::strncpy(cwd_, "/", sizeof(cwd_) - 1);
    cwd_[sizeof(cwd_) - 1] = '\0';
    depth_ = 0;
    refresh_();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (count_ == 0) return;
    switch (k.key) {
      case Key::Up:    menu_.up();   break;
      case Key::Down:  menu_.down(); break;
      case Key::Enter: enter_();     break;
      default: break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    char title[40];
    std::snprintf(title, sizeof(title), "Files %s", cwd_);
    d.draw_text(8, 4, title, kWhite, kJapanRed);

    if (count_ == 0) {
      d.draw_text(8, 40, "(empty)", kJapanRedDark, kWhite);
      d.flush();
      return;
    }

    const size_t cur = menu_.cursor();
    const size_t window = 7;
    const size_t start = (cur >= window) ? (cur - window + 1) : 0;
    for (size_t i = start; i < count_ && i < start + window; ++i) {
      const int y = 22 + static_cast<int>(i - start) * 13;
      const bool sel = (i == cur);
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : kBlack;
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
    if (!e.is_dir) return;
    if (std::strcmp(e.name, "..") == 0) { pop_(); return; }
    if (depth_ < kMaxDepth) {
      saved_cursors_[depth_] = menu_.cursor();
      ++depth_;
    }
    const size_t cwd_len  = std::strlen(cwd_);
    const size_t name_len = std::strlen(e.name);
    const size_t need     = cwd_len + 1 + name_len + 1;  // sep + nul
    if (need > sizeof(cwd_)) return;  // path too long, ignore
    if (at_root_()) {
      cwd_[1] = '\0';
      std::strncat(cwd_, e.name, sizeof(cwd_) - 2);
    } else {
      cwd_[cwd_len]     = '/';
      cwd_[cwd_len + 1] = '\0';
      std::strncat(cwd_, e.name, sizeof(cwd_) - cwd_len - 2);
    }
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

  IFs&    fs_;
  Menu    menu_;
  char    cwd_[128]                 = {0};
  FsEntry entries_[kMaxEntries]     = {};
  size_t  count_                    = 0;
  size_t  saved_cursors_[kMaxDepth] = {0};
  size_t  depth_                    = 0;
};

}  // namespace yui

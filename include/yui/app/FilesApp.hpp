#pragma once
// microSD directory browser. v0.0: read-only, single level (no recursion).
// Up/Down navigate, Enter into dirs (or no-op on files for now), Esc handled
// by the Shell.
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

  explicit FilesApp(IFs& fs) : fs_(fs), menu_(0) {}
  const char* name() const override { return "Files"; }

  void on_enter(Hal& /*hal*/) override {
    fs_.init();
    std::strncpy(cwd_, "/", sizeof(cwd_) - 1);
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

private:
  void refresh_() {
    const int n = fs_.list(cwd_, entries_, kMaxEntries);
    count_      = (n < 0) ? 0 : static_cast<size_t>(n);
    menu_.set_count(count_);
  }
  void enter_() {
    const auto& e = entries_[menu_.cursor()];
    if (!e.is_dir) return;
    // Path concat: ensure single slash between segments.
    char next[128];
    if (std::strcmp(cwd_, "/") == 0)
      std::snprintf(next, sizeof(next), "/%s", e.name);
    else
      std::snprintf(next, sizeof(next), "%s/%s", cwd_, e.name);
    std::strncpy(cwd_, next, sizeof(cwd_) - 1);
    cwd_[sizeof(cwd_) - 1] = '\0';
    refresh_();
  }

  IFs&    fs_;
  Menu    menu_;
  char    cwd_[128]              = {0};
  FsEntry entries_[kMaxEntries]  = {};
  size_t  count_                 = 0;
};

}  // namespace yui

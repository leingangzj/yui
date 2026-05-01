#pragma once
// Tiny line-oriented notes editor. One file at /yui-notes.txt on SD. The
// buffer is kept short (capped at kBufCap) — this is a notes app, not a
// word processor.
#include "yui/app/App.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/types.hpp"
#include <cstring>

namespace yui {

class NotesApp : public App {
public:
  static constexpr size_t      kBufCap = 1024;
  static constexpr const char* kPath   = "/yui-notes.txt";

  explicit NotesApp(IFs& fs) : fs_(fs) {}
  const char* name() const override { return "Notes"; }

  void on_enter(Hal& /*hal*/) override {
    fs_.init();
    len_ = 0;
    if (fs_.exists(kPath)) {
      const int n = fs_.read_all(kPath, buf_, kBufCap);
      if (n > 0) len_ = static_cast<size_t>(n);
    }
    buf_[len_]  = '\0';
    dirty_      = false;
    saved_flash_ = 0;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Backspace) {
      if (len_ > 0) { --len_; buf_[len_] = '\0'; dirty_ = true; }
      return;
    }
    if (k.key == Key::Enter) {
      append_('\n');
      return;
    }
    if (k.key == Key::Space) {
      append_(' ');
      return;
    }
    if (k.key == Key::Char && k.ch >= 0x20 && k.ch < 0x7F) {
      append_(k.ch);
      return;
    }
    // Tab = save (no Cmd/Ctrl shortcuts on the Cardputer kbd).
    if (k.key == Key::Tab) save_();
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, dirty_ ? "Notes *" : "Notes",  kWhite, kJapanRed);
    d.draw_text(d.width() - 60, 4, "Tab=save", kWhite, kJapanRed);

    // Render last ~7 lines of buf_ (height 14 each, ~7 fit in 95 px).
    constexpr int kVisibleLines = 7;
    const char* line_starts[kVisibleLines + 32]{};
    int total = 0;
    line_starts[total++] = buf_;
    for (size_t i = 0; i < len_ && total < static_cast<int>(sizeof(line_starts)/sizeof(*line_starts)); ++i) {
      if (buf_[i] == '\n' && i + 1 < len_) line_starts[total++] = buf_ + i + 1;
    }
    int start_idx = total > kVisibleLines ? total - kVisibleLines : 0;
    for (int i = start_idx; i < total; ++i) {
      const char* p   = line_starts[i];
      const char* end = (i + 1 < total) ? line_starts[i + 1] - 1 : buf_ + len_;
      char line[40];
      const size_t n = std::min<size_t>(sizeof(line) - 1, static_cast<size_t>(end - p));
      std::memcpy(line, p, n);
      line[n] = '\0';
      const int y = 22 + (i - start_idx) * 14;
      d.draw_text(4, y, line, kBlack, kWhite);
    }

    // Cursor as a small block at end of last line.
    if (saved_flash_ > 0) {
      d.draw_text(d.width() - 50, d.height() - 14, "saved", kJapanRed, kWhite);
    }
    d.flush();
  }

  void tick(uint32_t /*now_ms*/) override {
    if (saved_flash_ > 0) --saved_flash_;
  }

  // Test hooks
  const char* buffer() const { return buf_; }
  size_t      buffer_len() const { return len_; }
  bool        dirty() const { return dirty_; }

private:
  void append_(char c) {
    if (len_ + 1 >= kBufCap) return;
    buf_[len_++] = c;
    buf_[len_]   = '\0';
    dirty_       = true;
  }
  bool save_() {
    if (!fs_.write_all(kPath, buf_, len_)) return false;
    dirty_       = false;
    saved_flash_ = 60;  // ~2 s at 30 fps
    return true;
  }

  IFs&  fs_;
  char  buf_[kBufCap] = {0};
  size_t len_         = 0;
  bool  dirty_        = false;
  int   saved_flash_  = 0;
};

}  // namespace yui

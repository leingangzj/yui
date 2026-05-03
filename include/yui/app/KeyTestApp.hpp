#pragma once
// Live keypress display — useful for hardware bring-up and verifying the
// ADV keymap. Shows the last few KeyEvents with key enum, ch, modifiers,
// and down/up.
#include "yui/app/App.hpp"
#include "yui/types.hpp"
#include <cstdio>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class KeyTestApp : public App {
public:
  static constexpr size_t kHistory = 8;

  const char* name() const override { return "Key Test"; }
  Category    category() const override { return Category::System; }

  void on_enter(Hal& /*hal*/) override {
    count_ = 0;
    head_  = 0;
  }

  void on_key(KeyEvent k) override {
    log_[head_] = k;
    head_ = (head_ + 1) % kHistory;
    if (count_ < kHistory) ++count_;
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "Key Test", "press any key");

    if (count_ == 0) {
      d.draw_text(8, 40, "(waiting for input)", ui::kAccentDark, ui::kSurface);
      d.flush();
      return;
    }

    // Render newest first.
    for (size_t i = 0; i < count_; ++i) {
      const size_t idx = (head_ + kHistory - 1 - i) % kHistory;
      const KeyEvent& e = log_[idx];
      char line[48];
      char ch_repr[4];
      if (e.ch >= 0x20 && e.ch < 0x7F) { ch_repr[0] = e.ch; ch_repr[1] = '\0'; }
      else { std::snprintf(ch_repr, sizeof(ch_repr), "."); }
      std::snprintf(line, sizeof(line), "%s %-3s key=%-2d %c%c%c%c",
                    e.down ? "v" : "^", ch_repr, static_cast<int>(e.key),
                    e.shift ? 'S' : '-', e.ctrl ? 'C' : '-',
                    e.alt   ? 'A' : '-', e.fn   ? 'F' : '-');
      const int y = 22 + static_cast<int>(i) * 13;
      d.draw_text(4, y, line, ui::kOnSurface, ui::kSurface);
    }
    d.flush();
  }

  // Test hooks
  size_t count() const { return count_; }
  KeyEvent newest() const {
    const size_t idx = (head_ + kHistory - 1) % kHistory;
    return log_[idx];
  }

private:
  KeyEvent log_[kHistory] = {};
  size_t   count_ = 0;
  size_t   head_  = 0;
};

}  // namespace yui

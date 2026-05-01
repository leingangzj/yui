#pragma once
// IR remote — TV-B-Gone-style power-blast app. Cycles through a hardcoded
// list of NEC power codes, sending one each Enter press; long-press equivalent
// (hold Enter, but our v0.0 keymap doesn't support repeat) → blast all.
#include "yui/app/App.hpp"
#include "yui/hal/IIr.hpp"
#include "yui/types.hpp"
#include <cstdio>

namespace yui {

struct NecCode {
  const char* label;
  uint16_t    addr;
  uint16_t    cmd;
};

// Tiny starter set. Add to this as you collect codes for your devices.
constexpr NecCode kNecPresets[] = {
  {"Samsung TV power", 0xE0E0, 0x40BF},
  {"LG TV power",      0x20DF, 0x10EF},
  {"Sony TV power",    0xA90,  0x15},
  {"Panasonic power",  0x4004, 0x0BCD},
};
constexpr size_t kNecPresetCount = sizeof(kNecPresets) / sizeof(kNecPresets[0]);

class IrRemoteApp : public App {
public:
  explicit IrRemoteApp(IIr& ir) : ir_(ir) {}
  const char* name() const override { return "IR Remote"; }

  void on_enter(Hal& /*hal*/) override {
    ir_.init();
    cursor_ = 0;
    last_sent_count_ = ir_.sent_count();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Up   && cursor_ > 0) --cursor_;
    if (k.key == Key::Down && cursor_ + 1 < kNecPresetCount) ++cursor_;
    if (k.key == Key::Enter) {
      const auto& c = kNecPresets[cursor_];
      ir_.send_nec(c.addr, c.cmd, 38);
      last_sent_count_ = ir_.sent_count();
      flash_ = 30;
    }
  }

  void tick(uint32_t /*now_ms*/) override {
    if (flash_ > 0) --flash_;
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "IR Remote", kWhite, kJapanRed);

    for (size_t i = 0; i < kNecPresetCount; ++i) {
      const int y    = 22 + static_cast<int>(i) * 14;
      const bool sel = (i == cursor_);
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : kBlack;
      if (sel) d.fill_rect({0, y - 2, d.width(), 14}, bg);
      d.draw_text(8, y + 3, kNecPresets[i].label, fg, bg);
    }

    if (flash_ > 0) {
      d.draw_text(8, d.height() - 14, "blast!", kJapanRed, kWhite);
    } else {
      d.draw_text(8, d.height() - 14, "Enter to send", kJapanRedDark, kWhite);
    }
    d.flush();
  }

  // Test hooks
  size_t cursor() const { return cursor_; }

private:
  IIr&     ir_;
  size_t   cursor_          = 0;
  uint32_t last_sent_count_ = 0;
  int      flash_           = 0;
};

}  // namespace yui

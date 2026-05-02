#pragma once
// TvBGoneApp — sweep through a list of well-known NEC TV power codes
// and blast each one via the IR emitter. Functionally equivalent to
// the classic "TV-B-Gone" gadget. Pure NEC for v0.2; older RC5 sets
// are out of scope (the IRremoteESP8266 lib used in v0.1 supports
// them, can be added later).
//
// Tab toggles enabled. While enabled, tick() advances through the
// table once every kPerCodeMs, sending each entry exactly once. When
// the list is exhausted, enabled flips back off automatically.
#include "yui/app/App.hpp"
#include "yui/hal/IIr.hpp"
#include "yui/types.hpp"
#include <cstdint>
#include <cstdio>

namespace yui {

class TvBGoneApp : public App {
public:
  static constexpr uint32_t kPerCodeMs = 80;

  // Compact subset of the original tv-b-gone NA codes — Sony, Samsung,
  // LG, Vizio, Panasonic, Sharp, Toshiba, Philips, RCA. NEC-encodable
  // codes only. Real device fires ~70 codes; we approximate.
  struct Code { uint16_t addr; uint16_t cmd; };

  static const Code* codes(size_t& count) {
    static const Code k[] = {
      {0x01, 0x15},  // Sony power
      {0x07, 0x02},  // Samsung
      {0x04, 0x08},  // LG
      {0x10, 0x0E},  // Vizio
      {0x40, 0x12},  // Panasonic
      {0x40, 0x1A},  // Sharp
      {0x40, 0x14},  // Toshiba
      {0x00, 0x0C},  // Philips
      {0x05, 0x06},  // RCA
      {0x00, 0xA8},  // generic NEC1
      {0x00, 0x10},  // generic NEC2
      {0x80, 0x12},  // Hitachi
    };
    count = sizeof(k) / sizeof(k[0]);
    return k;
  }

  explicit TvBGoneApp(IIr& ir) : ir_(ir) {}

  const char* name() const override { return "TV-B-Gone"; }
  Category    category() const override { return Category::Tools; }

  void on_enter(Hal& hal) override {
    hal_           = &hal;
    enabled_       = false;
    idx_           = 0;
    last_send_ms_  = 0;
    sent_total_    = 0;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Tab || (k.key == Key::Enter && k.fn)) {
      enabled_ = !enabled_;
      if (enabled_) { idx_ = 0; last_send_ms_ = 0; sent_total_ = 0; }
    }
  }

  void tick(uint32_t now_ms) override {
    if (!enabled_) return;
    if (now_ms - last_send_ms_ < kPerCodeMs && last_send_ms_ != 0) return;
    last_send_ms_ = now_ms == 0 ? 1 : now_ms;
    size_t n = 0;
    const Code* list = codes(n);
    if (idx_ >= n) { enabled_ = false; return; }
    ir_.send_nec(list[idx_].addr, list[idx_].cmd);
    ++idx_;
    ++sent_total_;
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "TV-B-Gone", kWhite, kJapanRed);

    char line[40];
    size_t n = 0; codes(n);
    std::snprintf(line, sizeof(line), "State: %s", enabled_ ? "BLASTING" : "idle");
    d.draw_text(8, 24, line, enabled_ ? kJapanRedBright : kBlack, kWhite);
    std::snprintf(line, sizeof(line), "Sent:  %u / %u",
                  static_cast<unsigned>(sent_total_),
                  static_cast<unsigned>(n));
    d.draw_text(8, 44, line, kBlack, kWhite);
    d.draw_text(8, 64, "Tab: toggle", kJapanRed, kWhite);
    d.draw_text(8, d.height() - 14,
                "Use only on YOUR TVs",
                kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  bool   enabled()    const { return enabled_; }
  size_t idx()        const { return idx_; }
  size_t sent_total() const { return sent_total_; }

private:
  IIr&      ir_;
  Hal*      hal_           = nullptr;
  bool      enabled_       = false;
  size_t    idx_           = 0;
  size_t    sent_total_    = 0;
  uint32_t  last_send_ms_  = 0;
};

}  // namespace yui

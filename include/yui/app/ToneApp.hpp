#pragma once
// Tone player. Up/Down picks a preset; Enter plays it. Each preset is a
// short sequence of (freq_hz, duration_ms) pairs played sequentially via
// the IClock-driven tick loop (no blocking).
#include "yui/app/App.hpp"
#include "yui/hal/ISpeaker.hpp"
#include "yui/shell/Menu.hpp"
#include "yui/types.hpp"
#include <cstdint>
#include <cstdio>

namespace yui {

struct TonePresetNote { uint16_t freq_hz; uint16_t duration_ms; };

struct TonePreset {
  const char*           name;
  const TonePresetNote* notes;
  size_t                count;
};

namespace presets {
constexpr TonePresetNote kCMajor[] = {
  {262, 250}, {294, 250}, {330, 250}, {349, 250},
  {392, 250}, {440, 250}, {494, 250}, {523, 400},
};
constexpr TonePresetNote kArpeggio[] = {
  {523, 120}, {659, 120}, {784, 120}, {1047, 120},
  {784, 120}, {659, 120}, {523, 240},
};
constexpr TonePresetNote kBoot[] = {
  {880, 80}, {0, 40}, {1175, 80}, {0, 40}, {1568, 200},
};
constexpr TonePresetNote kErrorBeep[] = {
  {220, 200}, {0, 60}, {220, 200},
};
constexpr TonePreset kAll[] = {
  {"C major scale", kCMajor,    8},
  {"Arpeggio",      kArpeggio,  7},
  {"Boot chime",    kBoot,      5},
  {"Error beep",    kErrorBeep, 3},
};
constexpr size_t kCount = sizeof(kAll) / sizeof(kAll[0]);
}  // namespace presets

class ToneApp : public App {
public:
  explicit ToneApp(ISpeaker& spk) : spk_(spk), menu_(presets::kCount) {}
  const char* name() const override { return "Tone Player"; }

  void on_enter(Hal& /*hal*/) override {
    spk_.init();
    playing_       = false;
    note_idx_      = 0;
    next_note_ms_  = 0;
    last_ms_       = 0;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    switch (k.key) {
      case Key::Up:    menu_.up();   break;
      case Key::Down:  menu_.down(); break;
      case Key::Enter: start_();     break;
      case Key::Backspace: stop_();  break;
      default: break;
    }
  }

  void tick(uint32_t now_ms) override {
    last_ms_ = now_ms;
    if (!playing_) return;
    if (now_ms < next_note_ms_) return;
    const TonePreset& p = presets::kAll[menu_.cursor()];
    if (note_idx_ >= p.count) { playing_ = false; return; }
    const auto& n = p.notes[note_idx_];
    if (n.freq_hz > 0) spk_.tone(n.freq_hz, n.duration_ms);
    next_note_ms_ = now_ms + n.duration_ms;
    ++note_idx_;
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "Tone Player", kWhite, kJapanRed);
    d.draw_text(d.width() - 80, 4, playing_ ? "playing" : "Enter", kWhite, kJapanRed);

    const size_t cur = menu_.cursor();
    for (size_t i = 0; i < presets::kCount; ++i) {
      const int y = 22 + static_cast<int>(i) * 16;
      const bool sel = (i == cur);
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : kBlack;
      if (sel) d.fill_rect({0, y - 2, d.width(), 16}, bg);
      char line[40];
      std::snprintf(line, sizeof(line), "%s %s",
                    sel ? ">" : " ", presets::kAll[i].name);
      d.draw_text(4, y + 2, line, fg, bg);
    }
    d.draw_text(8, d.height() - 14, "Bksp=stop", kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  bool   playing() const { return playing_; }
  size_t cursor()  const { return menu_.cursor(); }
  size_t note_index() const { return note_idx_; }

private:
  void start_() {
    playing_      = true;
    note_idx_     = 0;
    next_note_ms_ = last_ms_;  // fire immediately on next tick
  }
  void stop_() {
    playing_ = false;
    spk_.stop();
  }

  ISpeaker& spk_;
  Menu      menu_;
  bool      playing_      = false;
  size_t    note_idx_     = 0;
  uint32_t  next_note_ms_ = 0;
  uint32_t  last_ms_      = 0;
};

}  // namespace yui

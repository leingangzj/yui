#pragma once
// Metronome. Up/Down adjusts BPM in 4-step increments (clamped 30..240),
// Enter starts/stops. Click is a 2 kHz blip (downbeat) or 1.5 kHz blip
// (offbeat). 4/4 by default; Tab cycles 2/4, 3/4, 4/4, 6/8.
#include "yui/app/App.hpp"
#include "yui/hal/ISpeaker.hpp"
#include "yui/types.hpp"
#include <cstdint>
#include <cstdio>

namespace yui {

class MetronomeApp : public App {
public:
  static constexpr int kBpmMin  = 30;
  static constexpr int kBpmMax  = 240;
  static constexpr int kBpmStep = 4;

  static constexpr int kSigs[][2] = { {2,4}, {3,4}, {4,4}, {6,8} };
  static constexpr size_t kSigCount = sizeof(kSigs) / sizeof(kSigs[0]);

  explicit MetronomeApp(ISpeaker& spk) : spk_(spk) {}
  const char* name() const override { return "Metronome"; }

  void on_enter(Hal& /*hal*/) override {
    spk_.init();
    running_  = false;
    beat_     = 0;
    last_ms_  = 0;
    next_ms_  = 0;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    switch (k.key) {
      case Key::Up:    set_bpm_(bpm_ + kBpmStep); break;
      case Key::Down:  set_bpm_(bpm_ - kBpmStep); break;
      case Key::Enter: toggle_(); break;
      case Key::Tab:
        if (!running_) sig_ = (sig_ + 1) % kSigCount;
        break;
      default: break;
    }
  }

  void tick(uint32_t now_ms) override {
    last_ms_ = now_ms;
    if (!running_) return;
    if (next_ms_ == 0) next_ms_ = now_ms;
    if (now_ms >= next_ms_) {
      const bool downbeat = (beat_ == 0);
      spk_.tone(downbeat ? 2000 : 1500, 30);
      beat_ = (beat_ + 1) % kSigs[sig_][0];
      next_ms_ += interval_ms_();
      if (next_ms_ <= now_ms) next_ms_ = now_ms + interval_ms_();
    }
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, running_ ? "Metronome ON" : "Metronome",
                kWhite, kJapanRed);

    char big[16];
    std::snprintf(big, sizeof(big), "%d BPM", bpm_);
    d.draw_text(d.width() / 2 - 30, 40, big, kBlack, kWhite);

    char sig[16];
    std::snprintf(sig, sizeof(sig), "%d/%d  beat %d", kSigs[sig_][0], kSigs[sig_][1], beat_ + 1);
    d.draw_text(8, d.height() - 28, sig, kJapanRedDark, kWhite);
    d.draw_text(8, d.height() - 14, "Up/Dn=BPM Ent=start Tab=sig",
                kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  int      bpm()      const { return bpm_; }
  bool     running()  const { return running_; }
  size_t   signature_idx() const { return sig_; }
  int      beat()     const { return beat_; }
  uint32_t interval_ms() const { return interval_ms_(); }

private:
  void set_bpm_(int b) {
    if (b < kBpmMin) b = kBpmMin;
    if (b > kBpmMax) b = kBpmMax;
    bpm_ = b;
    if (running_) {
      // Re-anchor next click so the new tempo takes effect smoothly.
      next_ms_ = last_ms_ + interval_ms_();
    }
  }
  void toggle_() {
    running_ = !running_;
    beat_    = 0;
    next_ms_ = running_ ? last_ms_ : 0;  // immediate downbeat on start
  }
  uint32_t interval_ms_() const { return 60000u / static_cast<uint32_t>(bpm_); }

  ISpeaker& spk_;
  int      bpm_    = 100;
  bool     running_ = false;
  size_t   sig_    = 2;  // default 4/4
  int      beat_   = 0;
  uint32_t last_ms_ = 0;
  uint32_t next_ms_ = 0;
};

}  // namespace yui

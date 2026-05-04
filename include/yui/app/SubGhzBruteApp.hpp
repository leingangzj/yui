#pragma once
// SubGhzBruteApp — fixed-code dictionary brute against sub-GHz
// remotes that don't use rolling codes.
//
// Walks a 24-bit code space (3 bytes) at a chosen freq + bitrate,
// transmitting each code as an OOK packet. The user picks a counter
// step and an end-condition (count or full sweep). Uses
// CC1101 packet-mode transmit for each candidate — the receiver
// either matches and unlocks, or doesn't.

#include "yui/app/App.hpp"
#include "yui/hal/ICc1101.hpp"
#include "yui/hal/IClock.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>

namespace yui {

class SubGhzBruteApp : public App {
public:
  enum class Mode { Idle, Running, Done, Stopped, CapMissing };

  SubGhzBruteApp(ICc1101* radio, IClock& clock)
    : radio_(radio), clock_(clock) {}
  const char* name() const override { return "Sub-GHz Brute"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& /*hal*/) override {
    mode_ = (radio_ && radio_->is_present()) ? Mode::Idle : Mode::CapMissing;
    counter_ = start_code_;
    sent_ = 0;
    last_step_ms_ = 0;
  }

  void on_exit() override {
    if (radio_) radio_->set_carrier(false);
    if (mode_ == Mode::Running) mode_ = Mode::Stopped;
  }

  void on_key(KeyEvent k) override {
    if (!k.down || mode_ == Mode::CapMissing) return;
    switch (k.key) {
      case Key::Left:
        if (freq_hz_ > 300'000'000) { freq_hz_ -= 100'000; tune_(); }
        break;
      case Key::Right:
        if (freq_hz_ < 928'000'000) { freq_hz_ += 100'000; tune_(); }
        break;
      case Key::Up:
        if (step_ < 256) step_ *= 2;
        break;
      case Key::Down:
        if (step_ > 1) step_ /= 2;
        break;
      case Key::Enter:
        if (mode_ == Mode::Idle || mode_ == Mode::Stopped || mode_ == Mode::Done) {
          counter_ = start_code_;
          sent_ = 0;
          mode_ = Mode::Running;
          if (radio_) {
            radio_->set_frequency_hz(freq_hz_);
            radio_->set_modulation(CcModulation::Ook);
            radio_->set_bitrate_bps(4800);
          }
        } else if (mode_ == Mode::Running) {
          mode_ = Mode::Stopped;
        }
        break;
      default: break;
    }
  }

  void tick(uint32_t now_ms) override {
    if (mode_ != Mode::Running || !radio_) return;
    // Pace TX at ~10 Hz so we don't lock the UI thread; receivers
    // typically need 50-100 ms between presses anyway. First tick of
    // a run fires immediately so the user sees the counter advance.
    if (sent_ > 0 && now_ms - last_step_ms_ < 100) return;
    last_step_ms_ = now_ms;

    // Encode counter as big-endian 3-byte payload.
    uint8_t pkt[3] = {
      static_cast<uint8_t>((counter_ >> 16) & 0xFF),
      static_cast<uint8_t>((counter_ >>  8) & 0xFF),
      static_cast<uint8_t>(counter_         & 0xFF),
    };
    radio_->transmit(pkt, sizeof(pkt));
    ++sent_;

    // Advance counter; stop when full 24-bit space scanned.
    counter_ += step_;
    if (counter_ >= end_code_) {
      mode_ = Mode::Done;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (mode_ == Mode::CapMissing) {
      ui::Chrome::cap_missing_dialog(d, "Sub-GHz Brute", "CC1101", "CS=13");
      return;
    }

    const char* sub = nullptr;
    switch (mode_) {
      case Mode::Running: sub = "RUNNING"; break;
      case Mode::Done:    sub = "DONE";    break;
      case Mode::Stopped: sub = "STOPPED"; break;
      default: break;
    }
    ui::Chrome::header(d, "Sub-GHz Brute", sub);

    char line[40];
    std::snprintf(line, sizeof(line), "%u.%03u MHz",
                  static_cast<unsigned>(freq_hz_ / 1'000'000),
                  static_cast<unsigned>((freq_hz_ / 1000) % 1000));
    ui::Chrome::stat(d, 0, "Freq", line);

    std::snprintf(line, sizeof(line), "%u", static_cast<unsigned>(step_));
    ui::Chrome::stat(d, 1, "Step", line);

    std::snprintf(line, sizeof(line), "0x%06X / 0x%06X",
                  static_cast<unsigned>(counter_),
                  static_cast<unsigned>(end_code_));
    ui::Chrome::stat(d, 2, "Code", line);

    std::snprintf(line, sizeof(line), "%u sent", sent_);
    ui::Chrome::stat(d, 3, "TX", line);

    ui::Chrome::footer(d, mode_ == Mode::Running ? "Enter:STOP  </>:tune"
                                                  : "Enter:start  ^v:step");
    d.flush();
  }

  // Test hooks
  Mode mode() const { return mode_; }
  uint32_t counter() const { return counter_; }
  uint32_t sent() const { return sent_; }
  uint32_t step() const { return step_; }

private:
  void tune_() { if (radio_) radio_->set_frequency_hz(freq_hz_); }

  ICc1101* radio_;
  IClock&  clock_;
  Mode     mode_ = Mode::Idle;
  uint32_t freq_hz_    = 433'920'000;
  uint32_t start_code_ = 0;
  uint32_t end_code_   = 0x01000000;   // 24-bit space
  uint32_t step_       = 1;
  uint32_t counter_    = 0;
  uint32_t sent_       = 0;
  uint32_t last_step_ms_ = 0;
};

}  // namespace yui

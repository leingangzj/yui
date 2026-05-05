#pragma once
// SubGhzCaptureApp — record raw sub-GHz signals to .sub files.
//
// Records timing edges (positive = TX-on, negative = TX-off, in µs)
// while the user holds Enter, stops on Enter again or auto-stops at
// the buffer cap. Saves to /sub/<freq>_<ts>.sub on the SD card via
// Yui's IFs HAL.
//
// CC1101 in raw async mode samples GDO0 directly — the host watches
// edge timestamps and records them. RadioLib doesn't expose a clean
// "raw sniff" mode, so for Phase 4 we reuse the chip's packet
// receiver to grab whatever it auto-decodes; full raw async + GPIO
// edge capture is a follow-up enhancement.

#include "yui/app/App.hpp"
#include "yui/hal/ICc1101.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/hal/IClock.hpp"
#include "yui/proto/SubFile.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>
#include <vector>

namespace yui {

class SubGhzCaptureApp : public App {
public:
  static constexpr std::size_t kMaxEdges = 4096;

  SubGhzCaptureApp(ICc1101* radio, IFs* fs, IClock& clock)
    : radio_(radio), fs_(fs), clock_(clock) {}

  const char* name() const override { return "Sub-GHz Capture"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& /*hal*/) override {
    if (!radio_ || !radio_->is_present()) {
      mode_ = Mode::CapMissing;
      return;
    }
    mode_ = Mode::Idle;
    edges_.clear();
    edges_.reserve(kMaxEdges);
    saved_path_[0] = 0;
    last_save_ms_ = 0;
    radio_->set_frequency_hz(freq_hz_);
    radio_->set_modulation(CcModulation::Ook);
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
      case Key::Enter:
        if (mode_ == Mode::Idle) start_capture_();
        else if (mode_ == Mode::Recording) stop_and_save_();
        break;
      default: break;
    }
  }

  void tick(uint32_t now_ms) override {
    if (mode_ != Mode::Recording || !radio_) return;
    // Collect any received bytes; convert each byte's transitions to
    // µs edges using a coarse 100 µs/bit assumption. Phase 4.5 swaps
    // this for true GDO0 edge capture.
    uint8_t buf[64];
    int n = radio_->receive(buf, sizeof(buf));
    if (n > 0 && edges_.size() + (n * 8) < kMaxEdges) {
      const uint32_t bit_us = 100;
      for (int i = 0; i < n; ++i) {
        for (int b = 7; b >= 0; --b) {
          const bool one = (buf[i] >> b) & 1;
          edges_.push_back(one ? static_cast<int32_t>(bit_us)
                               : -static_cast<int32_t>(bit_us));
        }
      }
    }
    // Auto-stop if we hit cap.
    if (edges_.size() >= kMaxEdges) stop_and_save_();
    // 30s safety stop.
    if (now_ms - rec_start_ms_ > 30'000) stop_and_save_();
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (mode_ == Mode::CapMissing) {
      ui::Chrome::cap_missing_dialog(d, "Sub-GHz Capture", "CC1101", "CS=13");
      return;
    }

    char sub[24];
    std::snprintf(sub, sizeof(sub), "%u.%03u MHz",
                  static_cast<unsigned>(freq_hz_ / 1'000'000),
                  static_cast<unsigned>((freq_hz_ / 1000) % 1000));
    ui::Chrome::radio_header(d, "Sub-GHz Capture", sub,
                             radio_ && radio_->is_present() ? 1 : 0, -1);

    const int y0 = ui::kBodyTopY + 4;
    char line[40];

    if (mode_ == Mode::Recording) {
      d.draw_text_styled(ui::kBodyPadX, y0, "● REC",
                         ui::kWarn, ui::kSurface, FontStyle::Title);
      std::snprintf(line, sizeof(line), "edges: %u",
                    static_cast<unsigned>(edges_.size()));
      d.draw_text_styled(ui::kBodyPadX, y0 + 22, line,
                         ui::kOnSurface, ui::kSurface, FontStyle::Body);
      const uint32_t elapsed = clock_.millis() - rec_start_ms_;
      std::snprintf(line, sizeof(line), "elapsed: %u s", elapsed / 1000);
      d.draw_text_styled(ui::kBodyPadX, y0 + 40, line,
                         ui::kHint, ui::kSurface, FontStyle::Caption);
    } else {
      d.draw_text_styled(ui::kBodyPadX, y0, "Ready",
                         ui::kAccent, ui::kSurface, FontStyle::Title);
      if (saved_path_[0]) {
        d.draw_text_styled(ui::kBodyPadX, y0 + 22, "Saved:",
                           ui::kHint, ui::kSurface, FontStyle::Caption);
        d.draw_text_styled(ui::kBodyPadX, y0 + 36, saved_path_,
                           ui::kOnSurface, ui::kSurface, FontStyle::Body);
      }
    }

    ui::Chrome::footer(d, mode_ == Mode::Recording
                          ? "Enter:stop+save  </>:tune"
                          : "Enter:record  </>:tune");
    d.flush();
  }

  // Test hooks
  enum class Mode { Idle, Recording, CapMissing };
  Mode mode() const { return mode_; }
  std::size_t edge_count() const { return edges_.size(); }
  uint32_t frequency_hz() const { return freq_hz_; }
  const char* last_saved_path() const { return saved_path_; }

private:
  void tune_() { if (radio_) radio_->set_frequency_hz(freq_hz_); }

  void start_capture_() {
    edges_.clear();
    rec_start_ms_ = clock_.millis();
    mode_ = Mode::Recording;
    if (radio_) radio_->set_frequency_hz(freq_hz_);
  }

  void stop_and_save_() {
    mode_ = Mode::Idle;
    if (!fs_ || edges_.empty()) return;
    proto::SubHeader h{};
    h.frequency_hz = freq_hz_;
    h.preset = proto::SubPreset::Ook650Async;
    std::string text = proto::write_sub(h, edges_.data(), edges_.size());
    // Filename = <freq_khz>_<uptime>.sub. /sub/ is created on demand.
    std::snprintf(saved_path_, sizeof(saved_path_),
                  "/sub/%u_%u.sub",
                  static_cast<unsigned>(freq_hz_ / 1000),
                  static_cast<unsigned>(clock_.millis() / 1000));
    fs_->write_all(saved_path_, text.data(), text.size());
    last_save_ms_ = clock_.millis();
  }

  ICc1101* radio_;
  IFs*     fs_;
  IClock&  clock_;
  Mode     mode_ = Mode::Idle;
  uint32_t freq_hz_ = 433'920'000;
  std::vector<int32_t> edges_;
  uint32_t rec_start_ms_ = 0;
  uint32_t last_save_ms_ = 0;
  char     saved_path_[40] = {};
};

}  // namespace yui

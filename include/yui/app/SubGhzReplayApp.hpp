#pragma once
// SubGhzReplayApp — pick a saved .sub file and transmit it.
//
// Lists /sub/*.sub on the SD via IFs, lets the user scroll, opens the
// file on Enter, parses out frequency + RAW timings, then drives the
// CC1101 to transmit.
//
// Phase 4.5: file picker + parsing + packet-mode transmit are live.
// Edge-by-edge async (true Flipper-style timing-accurate replay)
// requires direct GDO0 toggling and lands once we've validated the
// loop on hardware. For now we transmit the captured RAW byte stream
// via RadioLib's packet path — works for fixed-code remotes whose
// receiver tolerates the packet framing, which covers most cheap
// 433 MHz remotes.

#include "yui/app/App.hpp"
#include "yui/hal/ICc1101.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/proto/SubFile.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

namespace yui {

class SubGhzReplayApp : public App {
public:
  static constexpr int kMaxFiles = 32;
  static constexpr const char* kDir = "/sub";

  SubGhzReplayApp(ICc1101* radio, IFs* fs) : radio_(radio), fs_(fs) {}
  const char* name() const override { return "Sub-GHz Replay"; }
  Category    category() const override { return Category::Radio; }

  enum class Mode { CapMissing, NoFs, Browsing, Transmitting, Done, Error };

  void on_enter(Hal& /*hal*/) override {
    if (!radio_ || !radio_->is_present()) { mode_ = Mode::CapMissing; return; }
    if (!fs_) { mode_ = Mode::NoFs; return; }
    refresh_listing_();
    mode_ = Mode::Browsing;
    cursor_ = 0;
    status_[0] = 0;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (mode_ == Mode::Browsing) {
      if (k.key == Key::Up   && cursor_ > 0) --cursor_;
      if (k.key == Key::Down && cursor_ + 1 < count_) ++cursor_;
      if (k.key == Key::Enter && count_ > 0) {
        load_and_transmit_(entries_[cursor_].name);
      }
      return;
    }
    if (mode_ == Mode::Done || mode_ == Mode::Error) {
      if (k.key == Key::Enter || k.key == Key::Esc) {
        mode_ = Mode::Browsing;
      }
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    switch (mode_) {
      case Mode::CapMissing:
        ui::Chrome::header(d, "Sub-GHz Replay", "no cap");
        ui::Chrome::dialog(d, "Hydra not found",
                           "CC1101 did not respond. Re-seat the cap.",
                           "OK", nullptr, true);
        break;
      case Mode::NoFs:
        ui::Chrome::header(d, "Sub-GHz Replay");
        ui::Chrome::dialog(d, "No filesystem",
                           "SD card not mounted.", "OK", nullptr, true);
        break;
      case Mode::Browsing: render_browser_(d); break;
      case Mode::Transmitting:
        ui::Chrome::header(d, "Sub-GHz Replay", "TX...");
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 8,
                           "Transmitting...", ui::kAccent, ui::kSurface,
                           FontStyle::Title);
        break;
      case Mode::Done:
        ui::Chrome::header(d, "Sub-GHz Replay", "OK");
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 8, "Sent",
                           ui::kAccent, ui::kSurface, FontStyle::Title);
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 32, status_,
                           ui::kHint, ui::kSurface, FontStyle::Caption);
        break;
      case Mode::Error:
        ui::Chrome::header(d, "Sub-GHz Replay", "FAIL");
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 8, "Error",
                           ui::kWarn, ui::kSurface, FontStyle::Title);
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 32, status_,
                           ui::kHint, ui::kSurface, FontStyle::Caption);
        break;
    }
    if (mode_ == Mode::Done || mode_ == Mode::Error) {
      ui::Chrome::footer(d, "Enter:back");
    } else if (mode_ == Mode::Browsing) {
      ui::Chrome::footer(d, "Enter:send  Esc:back");
    }
    d.flush();
  }

  // Test hooks
  Mode mode() const { return mode_; }
  int  count() const { return count_; }
  int  cursor() const { return cursor_; }
  const char* status() const { return status_; }

private:
  void render_browser_(IDisplay& d) {
    char sub[24];
    std::snprintf(sub, sizeof(sub), "%d files", count_);
    ui::Chrome::header(d, "Sub-GHz Replay", sub);
    if (count_ == 0) {
      d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 8,
                         "No .sub files in /sub/",
                         ui::kHint, ui::kSurface, FontStyle::Body);
      d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 28,
                         "Use SubGhzCapture to record one.",
                         ui::kHint, ui::kSurface, FontStyle::Caption);
      return;
    }
    const int window = 4;
    const int start = (cursor_ >= window) ? (cursor_ - window + 1) : 0;
    for (int i = start; i < count_ && i < start + window; ++i) {
      ui::Chrome::list_row(d, i - start, entries_[i].name, i == cursor_);
    }
    if (count_ > window) {
      ui::Chrome::scrollbar(d, ui::kBodyTopY,
                            ui::kListRowH * window,
                            count_, window, start);
    }
  }

  void refresh_listing_() {
    count_ = 0;
    if (!fs_) return;
    FsEntry tmp[kMaxFiles];
    int n = fs_->list(kDir, tmp, kMaxFiles);
    if (n <= 0) return;
    for (int i = 0; i < n && count_ < kMaxFiles; ++i) {
      const std::size_t L = std::strlen(tmp[i].name);
      if (L >= 4 && std::strcmp(tmp[i].name + L - 4, ".sub") == 0) {
        entries_[count_++] = tmp[i];
      }
    }
  }

  void load_and_transmit_(const char* name) {
    if (!radio_ || !fs_) { mode_ = Mode::Error; std::strncpy(status_, "no hal", sizeof(status_) - 1); return; }
    char path[128];
    std::snprintf(path, sizeof(path), "%s/%s", kDir, name);
    // 4 KB covers ~512 RAW edges in the Flipper text format — plenty
    // for typical fixed-code remotes (~50-200 edges) and even larger
    // weather-sensor captures. Static so we don't blow stack on the
    // app-runner task.
    static char buf[4 * 1024];
    int n = fs_->read_all(path, buf, sizeof(buf));
    if (n <= 0) {
      mode_ = Mode::Error;
      std::strncpy(status_, "read failed", sizeof(status_) - 1);
      return;
    }
    proto::SubHeader h{};
    std::vector<int32_t> timings;
    bool ok = proto::read_sub_to_vector(buf, static_cast<std::size_t>(n),
                                        h, timings);
    if (!ok || timings.empty()) {
      mode_ = Mode::Error;
      std::strncpy(status_, "parse failed", sizeof(status_) - 1);
      return;
    }
    mode_ = Mode::Transmitting;
    radio_->set_frequency_hz(h.frequency_hz);
    radio_->set_modulation(CcModulation::Ook);
    // Pack the RAW timings as bytes (1 bit per pulse, sign → on/off).
    // Phase 4.5: this is the byte-stream path — fast, but receivers
    // expecting tight async timing will disagree. Phase 4.6 adds true
    // edge-toggle TX.
    std::vector<uint8_t> bytes;
    bytes.reserve((timings.size() + 7) / 8);
    uint8_t cur = 0;
    int bit = 7;
    for (int32_t t : timings) {
      const bool on = t > 0;
      if (on) cur |= (1 << bit);
      if (--bit < 0) { bytes.push_back(cur); cur = 0; bit = 7; }
    }
    if (bit != 7) bytes.push_back(cur);
    bool tx_ok = bytes.empty() ? true
                               : radio_->transmit(bytes.data(), bytes.size());
    if (tx_ok) {
      mode_ = Mode::Done;
      std::snprintf(status_, sizeof(status_), "%u edges", static_cast<unsigned>(timings.size()));
    } else {
      mode_ = Mode::Error;
      std::strncpy(status_, "tx failed", sizeof(status_) - 1);
    }
  }

  ICc1101* radio_;
  IFs*     fs_;
  Mode     mode_ = Mode::Browsing;
  int      cursor_ = 0;
  int      count_ = 0;
  FsEntry  entries_[kMaxFiles];
  char     status_[40] = {};
};

}  // namespace yui

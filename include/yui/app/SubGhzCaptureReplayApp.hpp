#pragma once
// SubGhzCaptureReplayApp — Flipper-style capture-and-replay in one app.
// Tab swaps Read (live capture from CC1101) ↔ Saved (browse + transmit
// .sub files from /sub/ on the SD).
//
// Same hardware (CC1101 via ICc1101), same file format (.sub via
// proto::SubFile), same defaults (OOK 650 async). Used to be two apps;
// merged in 4.13.2 since record-then-replay is one workflow.

#include "yui/app/App.hpp"
#include "yui/hal/ICc1101.hpp"
#include "yui/hal/IClock.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/proto/SubFile.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace yui {

class SubGhzCaptureReplayApp : public App {
 public:
  enum class View { Read, Saved };
  enum class TopMode { CapMissing, NoFs, Active };
  enum class CaptureMode { Idle, Recording };
  enum class ReplayMode { Browsing, Transmitting, Done, Error };

  static constexpr std::size_t kMaxEdges = 4096;
  static constexpr int         kMaxFiles = 32;
  static constexpr const char* kDir      = "/sub";

  SubGhzCaptureReplayApp(ICc1101* radio, IFs* fs, IClock& clock)
      : radio_(radio), fs_(fs), clock_(clock) {}

  const char* name() const override { return "Sub-GHz Capt/Replay"; }
  Category    category() const override { return Category::Radio; }
  const assets::IconRef* icon() const override { return &assets::icons::kCapture(); }

  void on_enter(Hal& /*hal*/) override {
    if (!radio_ || !radio_->is_present()) { top_ = TopMode::CapMissing; return; }
    if (!fs_)                              { top_ = TopMode::NoFs;       return; }
    top_     = TopMode::Active;
    view_    = View::Read;
    cap_     = CaptureMode::Idle;
    rep_     = ReplayMode::Browsing;
    cursor_  = 0;
    edges_.clear();
    edges_.reserve(kMaxEdges);
    saved_path_[0] = 0;
    status_[0]     = 0;
    refresh_listing_();
    radio_->set_frequency_hz(freq_hz_);
    radio_->set_modulation(CcModulation::Ook);
  }

  void on_key(KeyEvent k) override {
    if (!k.down || top_ != TopMode::Active) return;

    // Tab swaps view, but only when neither half is mid-action.
    if (k.key == Key::Tab && cap_ != CaptureMode::Recording &&
        rep_ != ReplayMode::Transmitting) {
      view_ = (view_ == View::Read) ? View::Saved : View::Read;
      if (view_ == View::Saved) refresh_listing_();
      return;
    }

    if (view_ == View::Read) on_key_capture_(k);
    else                     on_key_replay_(k);
  }

  void tick(uint32_t now_ms) override {
    if (top_ != TopMode::Active || view_ != View::Read) return;
    if (cap_ != CaptureMode::Recording || !radio_) return;
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
    if (edges_.size() >= kMaxEdges)            stop_and_save_();
    if (now_ms - rec_start_ms_ > 30'000)       stop_and_save_();
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (top_ == TopMode::CapMissing) {
      ui::Chrome::cap_missing_dialog(d, "Sub-GHz Capt/Replay", "CC1101", "CS=13");
      return;
    }
    if (top_ == TopMode::NoFs) {
      ui::Chrome::radio_header(d, "Sub-GHz Capt/Replay", nullptr,
                               radio_ && radio_->is_present() ? 1 : 0, -1);
      ui::Chrome::dialog(d, "No filesystem", "SD card not mounted.",
                         "OK", nullptr, true);
      d.flush();
      return;
    }

    if (view_ == View::Read) render_capture_(d);
    else                     render_replay_(d);

    const char* foot = nullptr;
    if (view_ == View::Read) {
      foot = (cap_ == CaptureMode::Recording) ? "Enter:stop+save  </>:tune"
                                              : "Tab:saved Enter:rec </>:tune";
    } else {
      switch (rep_) {
        case ReplayMode::Browsing:     foot = "Tab:read Enter:send Esc:back"; break;
        case ReplayMode::Transmitting: foot = "TX in progress…";              break;
        default:                       foot = "Enter:back Tab:read";          break;
      }
    }
    ui::Chrome::footer(d, foot);
    d.flush();
  }

  // Test hooks
  View         view()         const { return view_; }
  TopMode      top_mode()     const { return top_; }
  CaptureMode  capture_mode() const { return cap_; }
  ReplayMode   replay_mode()  const { return rep_; }
  std::size_t  edge_count()   const { return edges_.size(); }
  uint32_t     frequency_hz() const { return freq_hz_; }
  int          file_count()   const { return count_; }
  int          cursor()       const { return cursor_; }
  const char*  status()       const { return status_; }
  const char*  last_saved_path() const { return saved_path_; }
  void         set_view(View v) {
    view_ = v;
    if (v == View::Saved) refresh_listing_();
  }

 private:
  void on_key_capture_(KeyEvent k) {
    switch (k.key) {
      case Key::Left:
        if (freq_hz_ > 300'000'000) { freq_hz_ -= 100'000; tune_(); }
        break;
      case Key::Right:
        if (freq_hz_ < 928'000'000) { freq_hz_ += 100'000; tune_(); }
        break;
      case Key::Enter:
        if      (cap_ == CaptureMode::Idle)      start_capture_();
        else if (cap_ == CaptureMode::Recording) stop_and_save_();
        break;
      default: break;
    }
  }

  void on_key_replay_(KeyEvent k) {
    if (rep_ == ReplayMode::Browsing) {
      if (k.key == Key::Up   && cursor_ > 0)              --cursor_;
      if (k.key == Key::Down && cursor_ + 1 < count_)     ++cursor_;
      if (k.key == Key::Enter && count_ > 0)
        load_and_transmit_(entries_[cursor_].name);
      return;
    }
    if (rep_ == ReplayMode::Done || rep_ == ReplayMode::Error) {
      if (k.key == Key::Enter || k.key == Key::Esc) rep_ = ReplayMode::Browsing;
    }
  }

  void render_capture_(IDisplay& d) {
    char sub[24];
    std::snprintf(sub, sizeof(sub), "%u.%03u MHz",
                  static_cast<unsigned>(freq_hz_ / 1'000'000),
                  static_cast<unsigned>((freq_hz_ / 1000) % 1000));
    ui::Chrome::radio_header(d, "Sub-GHz · Read", sub,
                             radio_ && radio_->is_present() ? 1 : 0, -1);
    const int y0 = ui::kBodyTopY + 4;
    char line[40];
    if (cap_ == CaptureMode::Recording) {
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
  }

  void render_replay_(IDisplay& d) {
    char sub[24];
    switch (rep_) {
      case ReplayMode::Browsing:
        std::snprintf(sub, sizeof(sub), "Saved · %d", count_);
        ui::Chrome::radio_header(d, "Sub-GHz · Saved", sub,
                                 radio_ && radio_->is_present() ? 1 : 0, -1);
        if (count_ == 0) {
          d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 8,
                             "No .sub files in /sub/",
                             ui::kHint, ui::kSurface, FontStyle::Body);
          d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 28,
                             "Tab back to Read to record.",
                             ui::kHint, ui::kSurface, FontStyle::Caption);
          return;
        }
        {
          const int window = 4;
          const int start  = (cursor_ >= window) ? (cursor_ - window + 1) : 0;
          for (int i = start; i < count_ && i < start + window; ++i) {
            ui::Chrome::list_row(d, i - start, entries_[i].name, i == cursor_);
          }
          if (count_ > window) {
            ui::Chrome::scrollbar(d, ui::kBodyTopY,
                                  ui::kListRowH * window,
                                  count_, window, start);
          }
        }
        break;
      case ReplayMode::Transmitting:
        ui::Chrome::radio_header(d, "Sub-GHz · Saved", "TX...",
                                 radio_ && radio_->is_present() ? 1 : 0, -1);
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 8,
                           "Transmitting...", ui::kAccent, ui::kSurface,
                           FontStyle::Title);
        break;
      case ReplayMode::Done:
        ui::Chrome::radio_header(d, "Sub-GHz · Saved", "OK",
                                 radio_ && radio_->is_present() ? 1 : 0, -1);
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 8, "Sent",
                           ui::kAccent, ui::kSurface, FontStyle::Title);
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 32, status_,
                           ui::kHint, ui::kSurface, FontStyle::Caption);
        break;
      case ReplayMode::Error:
        ui::Chrome::radio_header(d, "Sub-GHz · Saved", "FAIL",
                                 radio_ && radio_->is_present() ? 1 : 0, -1);
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 8, "Error",
                           ui::kWarn, ui::kSurface, FontStyle::Title);
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 32, status_,
                           ui::kHint, ui::kSurface, FontStyle::Caption);
        break;
    }
  }

  void tune_() { if (radio_) radio_->set_frequency_hz(freq_hz_); }

  void start_capture_() {
    edges_.clear();
    rec_start_ms_ = clock_.millis();
    cap_ = CaptureMode::Recording;
    if (radio_) radio_->set_frequency_hz(freq_hz_);
  }

  void stop_and_save_() {
    cap_ = CaptureMode::Idle;
    if (!fs_ || edges_.empty()) return;
    proto::SubHeader h{};
    h.frequency_hz = freq_hz_;
    h.preset       = proto::SubPreset::Ook650Async;
    std::string text = proto::write_sub(h, edges_.data(), edges_.size());
    std::snprintf(saved_path_, sizeof(saved_path_),
                  "/sub/%u_%u.sub",
                  static_cast<unsigned>(freq_hz_ / 1000),
                  static_cast<unsigned>(clock_.millis() / 1000));
    fs_->write_all(saved_path_, text.data(), text.size());
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

  void load_and_transmit_(const char* fname) {
    if (!radio_ || !fs_) {
      rep_ = ReplayMode::Error;
      std::strncpy(status_, "no hal", sizeof(status_) - 1);
      return;
    }
    char path[128];
    std::snprintf(path, sizeof(path), "%s/%s", kDir, fname);
    static char buf[4 * 1024];
    int n = fs_->read_all(path, buf, sizeof(buf));
    if (n <= 0) {
      rep_ = ReplayMode::Error;
      std::strncpy(status_, "read failed", sizeof(status_) - 1);
      return;
    }
    proto::SubHeader h{};
    std::vector<int32_t> timings;
    bool ok = proto::read_sub_to_vector(buf, static_cast<std::size_t>(n),
                                        h, timings);
    if (!ok || timings.empty()) {
      rep_ = ReplayMode::Error;
      std::strncpy(status_, "parse failed", sizeof(status_) - 1);
      return;
    }
    rep_ = ReplayMode::Transmitting;
    radio_->set_frequency_hz(h.frequency_hz);
    radio_->set_modulation(CcModulation::Ook);
    bool tx_ok = radio_->transmit_raw_edges(timings.data(), timings.size());
    if (tx_ok) {
      rep_ = ReplayMode::Done;
      std::snprintf(status_, sizeof(status_), "%u edges",
                    static_cast<unsigned>(timings.size()));
    } else {
      rep_ = ReplayMode::Error;
      std::strncpy(status_, "tx failed", sizeof(status_) - 1);
    }
  }

  ICc1101* radio_;
  IFs*     fs_;
  IClock&  clock_;
  TopMode      top_  = TopMode::Active;
  View         view_ = View::Read;
  CaptureMode  cap_  = CaptureMode::Idle;
  ReplayMode   rep_  = ReplayMode::Browsing;
  uint32_t     freq_hz_ = 433'920'000;
  std::vector<int32_t> edges_;
  uint32_t     rec_start_ms_ = 0;
  char         saved_path_[40] = {};
  int          cursor_ = 0;
  int          count_  = 0;
  FsEntry      entries_[kMaxFiles];
  char         status_[40] = {};
};

}  // namespace yui

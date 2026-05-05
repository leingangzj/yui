#pragma once
// MousejackApp — Logitech Unifying / generic 2.4 GHz wireless HID
// keystroke injection (Bastille's Mousejack research).
//
// Two-phase workflow:
//   1. SCAN — sweep channels 1..125, listen briefly on each. Any
//      address that responds gets stored as a candidate target.
//   2. INJECT — pick a target, send a stock HID payload (preset).
//
// Phase 4.6 lands the full state machine + UI + radio drive. The
// promiscuous-mode trick that lets the nRF24 sniff arbitrary
// addresses without pre-pairing is not in RadioLib's public API yet
// — until a hardware-test session validates a workaround, the scan
// uses standard pipe-0 listening (catches devices the user has
// pre-configured) rather than true promiscuous capture.

#include "yui/app/App.hpp"
#include "yui/hal/INrf24.hpp"
#include "yui/hal/IClock.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include "yui/util/FaradayMode.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class MousejackApp : public App {
public:
  enum class Mode { CapMissing, Scanning, TargetList, Injecting, Done };
  enum class ScanType { Targets, Generic };  // Generic = channel heatmap

  static constexpr int kMaxTargets   = 8;
  static constexpr int kAddrLen      = 5;
  static constexpr int kChannelCount = 126;

  // Default address Logitech Unifying receivers respond to before a
  // device is paired — the well-known "magic" prefix from Bastille's
  // research. Real-world targets vary; user can edit later.
  static constexpr uint8_t kSeedAddress[kAddrLen] = {0xBB, 0x0A, 0xDC, 0xA5, 0x75};

  // Stock HID payload — three keystrokes (Win + r, "calc", Enter) in
  // Logitech's encrypted Unifying format would go here. For Phase 4.6
  // we emit a placeholder pattern that the receiver will reject; user
  // edits later or loads a real .duck-equivalent payload.
  static constexpr uint8_t kPlaceholderPayload[] = {
    0x00, 0xC1, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  MousejackApp(INrf24* radio, IClock& clock)
    : radio_(radio), clock_(clock) {}

  const char* name() const override { return "Mousejack"; }
  Category    category() const override { return Category::Bluetooth; }

  void on_enter(Hal& /*hal*/) override {
    if (!radio_ || !radio_->is_present()) { mode_ = Mode::CapMissing; return; }
    mode_ = Mode::Scanning;
    target_count_ = 0;
    cursor_ = 0;
    ch_ = 0;
    sent_ = 0;
    for (auto& v : hits_) v = 0;
    radio_->set_data_rate(NrfDataRate::Rate2Mbps);
    // Try real promiscuous mode (Bastille technique). Falls back to
    // pipe-0 listen on the seed Logitech address if the backend
    // doesn't support it (native fakes return false).
    if (!radio_->set_promiscuous(true)) {
      radio_->set_address(kSeedAddress, kAddrLen);
    }
    radio_->start_listening();
  }

  void on_exit() override {
    if (radio_) {
      radio_->set_carrier(false);
      radio_->stop_listening();
      radio_->set_promiscuous(false);  // restore normal address mode
    }
  }

  void on_key(KeyEvent k) override {
    if (!k.down || mode_ == Mode::CapMissing) return;
    switch (mode_) {
      case Mode::Scanning:
        if (k.key == Key::Tab) {
          scan_type_ = (scan_type_ == ScanType::Targets) ? ScanType::Generic
                                                         : ScanType::Targets;
          // Reset state so the new scan view starts clean.
          for (auto& v : hits_) v = 0;
          target_count_ = 0;
          ch_ = 0;
          break;
        }
        if (k.key == Key::Enter) {
          if (radio_) radio_->stop_listening();
          if (scan_type_ == ScanType::Generic) {
            // Generic scan never produces "targets"; Enter just freezes.
            mode_ = Mode::Done;
          } else {
            mode_ = (target_count_ > 0) ? Mode::TargetList : Mode::Done;
          }
        }
        break;
      case Mode::TargetList:
        if (k.key == Key::Up   && cursor_ > 0) --cursor_;
        if (k.key == Key::Down && cursor_ + 1 < target_count_) ++cursor_;
        if (k.key == Key::Enter) start_inject_();
        if (k.key == Key::Esc)   mode_ = Mode::Scanning;
        break;
      case Mode::Done:
      case Mode::Injecting:
        if (k.key == Key::Enter || k.key == Key::Esc) mode_ = Mode::Scanning;
        break;
      default: break;
    }
  }

  void tick(uint32_t now_ms) override {
    if (!radio_) return;
    if (mode_ == Mode::Scanning) {
      if (scan_type_ == ScanType::Generic) {
        // Inherited Nrf24Scan behavior: walk many channels per tick,
        // accumulate per-channel hit counts with slow decay so peaks
        // fade if the source goes quiet.
        for (int i = 0; i < 10; ++i) {
          radio_->set_channel(static_cast<uint8_t>(ch_));
          radio_->start_listening();
          if (radio_->carrier_detected()) {
            if (hits_[ch_] < 255) ++hits_[ch_];
          } else if (hits_[ch_] > 0) {
            --hits_[ch_];
          }
          ch_ = (ch_ + 1) % kChannelCount;
        }
      } else {
        // Targets: same as before — one channel per tick, record any
        // address that responds, finalize after a full sweep.
        ch_ = (ch_ + 1) % kChannelCount;
        radio_->set_channel(static_cast<uint8_t>(ch_));
        if (radio_->carrier_detected() && target_count_ < kMaxTargets) {
          Target& t = targets_[target_count_++];
          std::memcpy(t.addr, kSeedAddress, kAddrLen);
          t.channel = ch_;
        }
        if (ch_ == 0 && target_count_ > 0) {
          radio_->stop_listening();
          mode_ = Mode::TargetList;
        }
      }
    } else if (mode_ == Mode::Injecting) {
      // Pace at ~50 ms per packet so the receiver has time to ingest.
      // Faraday Mode strips this to ~2 ms (~500 pkt/s) — useful for
      // stress-testing receiver buffers in a shielded room.
      const uint32_t pace = FaradayMode::is_active() ? 2u : 50u;
      if (now_ms - last_tx_ms_ < pace) return;
      last_tx_ms_ = now_ms;
      if (sent_ < static_cast<int>(sizeof(kPlaceholderPayload))) {
        radio_->transmit(kPlaceholderPayload + sent_, 1);
        ++sent_;
      } else {
        mode_ = Mode::Done;
      }
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    if (mode_ == Mode::CapMissing) {
      ui::Chrome::cap_missing_dialog(d, "Mousejack", "nRF24", "CS=6");
      return;
    }
    char sub[24];
    switch (mode_) {
      case Mode::Scanning:
        if (scan_type_ == ScanType::Generic) {
          ui::Chrome::radio_header(d, "Mousejack · Generic Scan",
                                   nullptr, -1,
                                   radio_ && radio_->is_present() ? 1 : 0);
          render_generic_heatmap_(d);
          ui::Chrome::footer(d, "Tab:targets  Enter:freeze  Esc:back");
        } else {
          std::snprintf(sub, sizeof(sub), "ch %d  %d found",
                        ch_, target_count_);
          ui::Chrome::radio_header(d, "Mousejack · Targets", sub, -1,
                                   radio_ && radio_->is_present() ? 1 : 0);
          d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 8,
                             "Scanning channels 0..125",
                             ui::kAccent, ui::kSurface, FontStyle::Title);
          d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 32,
                             "Press Enter to skip ahead.",
                             ui::kHint, ui::kSurface, FontStyle::Caption);
          ui::Chrome::footer(d, "Tab:generic  Enter:targets  Esc:back");
        }
        break;
      case Mode::TargetList:
        std::snprintf(sub, sizeof(sub), "%d targets", target_count_);
        ui::Chrome::radio_header(d, "Mousejack", sub, -1,
                                 radio_ && radio_->is_present() ? 1 : 0);
        for (int i = 0; i < target_count_; ++i) {
          char line[40];
          const auto& a = targets_[i].addr;
          std::snprintf(line, sizeof(line),
                        "ch %d  %02X:%02X:%02X:%02X:%02X",
                        targets_[i].channel,
                        a[0], a[1], a[2], a[3], a[4]);
          ui::Chrome::list_row(d, i, line, i == cursor_);
        }
        ui::Chrome::footer(d, "Enter:inject  Esc:rescan");
        break;
      case Mode::Injecting:
        ui::Chrome::radio_header(d, "Mousejack", "INJECTING", -1,
                                 radio_ && radio_->is_present() ? 1 : 0);
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 8,
                           "Sending payload...",
                           ui::kWarn, ui::kSurface, FontStyle::Title);
        std::snprintf(sub, sizeof(sub), "%d / %d bytes", sent_,
                      static_cast<int>(sizeof(kPlaceholderPayload)));
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 32, sub,
                           ui::kHint, ui::kSurface, FontStyle::Caption);
        ui::Chrome::footer(d, "Esc:abort");
        break;
      case Mode::Done:
        ui::Chrome::radio_header(d, "Mousejack", "DONE", -1,
                                 radio_ && radio_->is_present() ? 1 : 0);
        d.draw_text_styled(ui::kBodyPadX, ui::kBodyTopY + 8,
                           target_count_ > 0 ? "Inject complete" : "No targets",
                           ui::kAccent, ui::kSurface, FontStyle::Title);
        ui::Chrome::footer(d, "Enter:rescan  Esc:back");
        break;
      default: break;
    }
    d.flush();
  }

  // Test hooks
  Mode     mode() const { return mode_; }
  ScanType scan_type() const { return scan_type_; }
  void     set_scan_type(ScanType t) { scan_type_ = t; }
  int      target_count() const { return target_count_; }
  int      cursor() const { return cursor_; }
  int      current_channel() const { return ch_; }
  uint8_t  channel_hits(int ch) const {
    if (ch < 0 || ch >= kChannelCount) return 0;
    return hits_[ch];
  }
  // Effective inject pacing in ms (depends on FaradayMode).
  static uint32_t inject_pace_ms() {
    return FaradayMode::is_active() ? 2u : 50u;
  }

  // Test seam: let tests hand-build a target so we don't have to
  // shape the radio's carrier-detect path during scanning.
  void inject_target_for_test(const uint8_t* addr, std::size_t len, int ch) {
    if (target_count_ >= kMaxTargets) return;
    Target& t = targets_[target_count_++];
    std::memcpy(t.addr, addr, len < kAddrLen ? len : kAddrLen);
    t.channel = ch;
  }

private:
  struct Target {
    uint8_t addr[kAddrLen];
    int     channel;
  };

  void render_generic_heatmap_(IDisplay& d) {
    const int W       = d.width();
    const int top_y   = ui::kBodyTopY;
    const int chart_h = (ui::kFooterY - top_y) - 18;
    const int base_y  = top_y + chart_h;
    int peak_ch = 0;
    uint8_t peak_v = 0;
    for (int i = 0; i < kChannelCount; ++i) {
      if (hits_[i] > peak_v) { peak_v = hits_[i]; peak_ch = i; }
    }
    for (int i = 0; i < kChannelCount; ++i) {
      const int v = hits_[i];
      if (v == 0) continue;
      const int h  = (v * chart_h) / 32;
      const int hh = h > chart_h ? chart_h : h;
      const int x  = (i * W) / kChannelCount;
      const int next_x = ((i + 1) * W) / kChannelCount;
      const int bw = next_x - x - 1;
      const Color c = (i == peak_ch) ? ui::kWarn : ui::kAccent;
      d.fill_rect({x, base_y - hh, bw > 0 ? bw : 1, hh}, c);
    }
    char line[40];
    std::snprintf(line, sizeof(line),
                  "peak ch %d  %d MHz  hits=%u",
                  peak_ch, 2400 + peak_ch, peak_v);
    d.draw_text_styled(ui::kBodyPadX, base_y + 2, line,
                       ui::kHint, ui::kSurface, FontStyle::Caption);
  }

  void start_inject_() {
    if (!radio_ || target_count_ == 0) return;
    const auto& tgt = targets_[cursor_];
    radio_->set_channel(static_cast<uint8_t>(tgt.channel));
    radio_->set_address(tgt.addr, kAddrLen);
    radio_->stop_listening();
    sent_ = 0;
    last_tx_ms_ = clock_.millis();
    mode_ = Mode::Injecting;
  }

  INrf24*  radio_;
  IClock&  clock_;
  Mode     mode_      = Mode::Scanning;
  ScanType scan_type_ = ScanType::Targets;
  int      ch_        = 0;
  Target   targets_[kMaxTargets];
  int      target_count_ = 0;
  int      cursor_       = 0;
  int      sent_         = 0;
  uint32_t last_tx_ms_   = 0;
  uint8_t  hits_[kChannelCount] = {};
};

}  // namespace yui

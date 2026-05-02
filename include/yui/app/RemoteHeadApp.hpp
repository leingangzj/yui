#pragma once
// RemoteHeadApp — TH-D75 VFO frequency / mode editor over IRadioLink.
//
// In CommandMode, sends FQ/MD queries to read state and FQ/MD setters
// when the user changes them. Tab cycles between editable fields:
//   Field 0: VFO  (0=A, 1=B)
//   Field 1: Frequency in 1 MHz / 100 kHz / 10 kHz steps via Up/Down
//   Field 2: Mode (FM/NFM/AM/USB/LSB/CW)
//
// Up/Down adjust the active field; Enter writes the change to the radio.
// All flows exercised via FakeRadioLink in native tests.
#include "yui/app/App.hpp"
#include "yui/hal/IRadioLink.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class RemoteHeadApp : public App {
public:
  enum class Field : uint8_t { Vfo, Freq, Mode };

  static constexpr uint8_t kModeFM   = 0;
  static constexpr uint8_t kModeDV   = 1;
  static constexpr uint8_t kModeAM   = 2;
  static constexpr uint8_t kModeLSB  = 3;
  static constexpr uint8_t kModeUSB  = 4;
  static constexpr uint8_t kModeCW   = 5;
  static constexpr uint8_t kModeNFM  = 6;

  explicit RemoteHeadApp(IRadioLink& radio) : radio_(radio) {}

  const char* name() const override { return "Remote"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& hal) override {
    hal_   = &hal;
    field_ = Field::Vfo;
    refresh_();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    switch (k.key) {
      case Key::Tab:   cycle_field_();   break;
      case Key::Up:    bump_(+1);        break;
      case Key::Down:  bump_(-1);        break;
      case Key::Left:  step_idx_ = (step_idx_ + 2) % 3; break;
      case Key::Right: step_idx_ = (step_idx_ + 1) % 3; break;
      case Key::Enter: commit_();        break;
      default: break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "Remote Head", kWhite, kJapanRed);

    char line[40];
    int y = 24;
    auto row = [&](Field f, const char* fmt, auto val) {
      const bool sel = (f == field_);
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : kBlack;
      if (sel) d.fill_rect({0, y - 2, d.width(), 16}, bg);
      std::snprintf(line, sizeof(line), fmt, val);
      d.draw_text(8, y + 3, line, fg, bg);
      y += 16;
    };
    row(Field::Vfo,  "VFO:  %s",        vfo_ == 0 ? "A" : "B");
    row(Field::Freq, "Freq: %lu Hz",    static_cast<unsigned long>(freq_hz_));
    row(Field::Mode, "Mode: %s",        mode_label_(mode_));

    static const char* steps[] = {"100kHz", " 10kHz", "  1kHz"};
    std::snprintf(line, sizeof(line), "Step: %s", steps[step_idx_]);
    d.draw_text(8, y + 3, line, kJapanRedDark, kWhite);
    d.draw_text(8, d.height() - 14,
                "Tab:field Up/Dn:adj Enter:set",
                kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  Field    field()    const { return field_; }
  uint8_t  vfo()      const { return vfo_; }
  uint64_t freq_hz()  const { return freq_hz_; }
  uint8_t  mode()     const { return mode_; }

private:
  void cycle_field_() {
    field_ = static_cast<Field>((static_cast<int>(field_) + 1) % 3);
  }

  void bump_(int delta) {
    if (field_ == Field::Vfo)  vfo_ = (vfo_ ^ 1) & 1, (void)delta;
    else if (field_ == Field::Mode) {
      const int m = (static_cast<int>(mode_) + delta + 7) % 7;
      mode_ = static_cast<uint8_t>(m);
    } else {
      static const uint64_t step_hz[] = {100000ULL, 10000ULL, 1000ULL};
      const uint64_t step = step_hz[step_idx_];
      if (delta > 0) freq_hz_ += step;
      else if (freq_hz_ >= step) freq_hz_ -= step;
    }
  }

  void commit_() {
    if (radio_.state() != RadioLinkState::CommandMode) return;
    char cmd[32];
    char resp[40] = {0};
    if (field_ == Field::Vfo)  return;  // VFO change is implicit on next FQ/MD
    if (field_ == Field::Freq) {
      std::snprintf(cmd, sizeof(cmd), "FQ %u,%010llu",
                    static_cast<unsigned>(vfo_),
                    static_cast<unsigned long long>(freq_hz_));
    } else {
      std::snprintf(cmd, sizeof(cmd), "MD %u,%u",
                    static_cast<unsigned>(vfo_),
                    static_cast<unsigned>(mode_));
    }
    radio_.command(cmd, resp, sizeof(resp), 1000);
  }

  void refresh_() {
    if (radio_.state() != RadioLinkState::CommandMode) return;
    char cmd[16], resp[40] = {0};
    std::snprintf(cmd, sizeof(cmd), "FQ %u", static_cast<unsigned>(vfo_));
    if (radio_.command(cmd, resp, sizeof(resp), 1000)) {
      // Expected reply: "FQ 0,0146520000"
      const char* comma = std::strchr(resp, ',');
      if (comma) freq_hz_ = std::strtoull(comma + 1, nullptr, 10);
    }
    std::snprintf(cmd, sizeof(cmd), "MD %u", static_cast<unsigned>(vfo_));
    if (radio_.command(cmd, resp, sizeof(resp), 1000)) {
      const char* comma = std::strchr(resp, ',');
      if (comma) mode_ = static_cast<uint8_t>(std::strtol(comma + 1, nullptr, 10));
    }
  }

  static const char* mode_label_(uint8_t m) {
    switch (m) {
      case kModeFM:  return "FM";
      case kModeDV:  return "DV";
      case kModeAM:  return "AM";
      case kModeLSB: return "LSB";
      case kModeUSB: return "USB";
      case kModeCW:  return "CW";
      case kModeNFM: return "NFM";
      default:       return "?";
    }
  }

  IRadioLink& radio_;
  Hal*        hal_      = nullptr;
  Field       field_    = Field::Vfo;
  uint8_t     vfo_      = 0;
  uint64_t    freq_hz_  = 0;
  uint8_t     mode_     = kModeFM;
  uint8_t     step_idx_ = 1;   // 10 kHz default
};

}  // namespace yui

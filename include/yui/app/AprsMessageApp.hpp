#pragma once
// AprsMessageApp — compose and send a short APRS message via the
// TH-D75's KISS TNC. v0.2 stretch goal.
//
// APRS message info-field format (DTI ':'):
//   :ADDRESSEE:Message text
// Where ADDRESSEE is exactly 9 chars (space-padded). No msg-ID
// support in v0.2 (no ack tracking).
//
// The source callsign comes from NVS key `tx.call` (with optional
// `tx.ssid`). If empty, the app surfaces a hint and disables sending.
//
// Frame is built directly (no generic ax25::encode helper yet) and
// KISS-wrapped, then handed to IRadioLink::kiss_write().
#include "yui/app/App.hpp"
#include "yui/hal/IRadioLink.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/proto/Kiss.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class AprsMessageApp : public App {
public:
  enum class Field : uint8_t { To, Msg };
  enum class Status : uint8_t { Idle, Sending, Sent, Failed, NoCallsign };

  static constexpr size_t kToCap  = 10;   // 9 chars + NUL
  static constexpr size_t kMsgCap = 64;

  AprsMessageApp(IRadioLink& radio, IStorage& store)
      : radio_(radio), store_(store) {}

  const char* name() const override { return "APRS TX"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& hal) override {
    hal_      = &hal;
    field_    = Field::To;
    to_len_   = 0;
    msg_len_  = 0;
    to_[0]    = '\0';
    msg_[0]   = '\0';
    status_   = Status::Idle;
    if (!load_call_()) status_ = Status::NoCallsign;
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (status_ == Status::NoCallsign) return;
    if (k.key == Key::Tab) { field_ = (field_ == Field::To) ? Field::Msg : Field::To; return; }
    if (k.key == Key::Enter) { send_(); return; }
    if (k.key == Key::Backspace) {
      char* buf  = (field_ == Field::To) ? to_  : msg_;
      size_t* l  = (field_ == Field::To) ? &to_len_ : &msg_len_;
      if (*l > 0) { --*l; buf[*l] = '\0'; }
      return;
    }
    if (k.key == Key::Space) { append_(' '); return; }
    if (k.key == Key::Char && k.ch >= 0x20 && k.ch < 0x7F) append_(k.ch);
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "APRS TX", kWhite, kJapanRed);

    if (status_ == Status::NoCallsign) {
      d.draw_text(8, 36, "Set tx.call in NVS", kJapanRedBright, kWhite);
      d.draw_text(8, 56, "(your station's call)", kJapanRedDark, kWhite);
      d.draw_text(8, d.height() - 14, "Esc: back", kJapanRedDark, kWhite);
      d.flush();
      return;
    }

    char line[40];
    int y = 22;
    std::snprintf(line, sizeof(line), "From: %s", call_);
    d.draw_text(8, y, line, kJapanRedDark, kWhite); y += 14;

    auto field = [&](Field f, const char* label, const char* val) {
      const bool sel = (f == field_);
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : kBlack;
      if (sel) d.fill_rect({0, y - 2, d.width(), 16}, bg);
      std::snprintf(line, sizeof(line), "%s %s", label, val);
      d.draw_text(8, y + 3, line, fg, bg);
      y += 16;
    };
    field(Field::To,  "To: ", to_);
    field(Field::Msg, "Msg:", msg_);

    const char* status =
      status_ == Status::Sending ? "Sending..." :
      status_ == Status::Sent    ? "Sent OK"    :
      status_ == Status::Failed  ? "Send FAILED": "";
    if (status[0]) d.draw_text(8, y + 4, status,
        status_ == Status::Failed ? kJapanRedBright : kJapanRed, kWhite);

    d.draw_text(8, d.height() - 14, "Tab:field Enter:send",
                kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  const char* to_text()   const { return to_; }
  const char* msg_text()  const { return msg_; }
  Status      status()    const { return status_; }
  Field       field()     const { return field_; }

private:
  void append_(char c) {
    char* buf  = (field_ == Field::To) ? to_  : msg_;
    size_t* l  = (field_ == Field::To) ? &to_len_ : &msg_len_;
    const size_t cap = (field_ == Field::To) ? kToCap : kMsgCap;
    if (*l + 1 < cap) { buf[(*l)++] = c; buf[*l] = '\0'; }
  }

  bool load_call_() {
    char buf[12] = {0};
    if (!store_.get_str("tx.call", buf, sizeof(buf)) || buf[0] == 0) return false;
    int32_t ssid = 0;
    store_.get_int("tx.ssid", ssid, 0);
    if (ssid > 0) std::snprintf(call_, sizeof(call_), "%s-%d",
                                buf, static_cast<int>(ssid));
    else          std::snprintf(call_, sizeof(call_), "%s", buf);
    return true;
  }

  void send_() {
    if (radio_.state() != RadioLinkState::KissMode) {
      radio_.enter_kiss(0);
    }
    if (radio_.state() != RadioLinkState::KissMode) {
      status_ = Status::Failed; return;
    }
    if (to_len_ == 0 || msg_len_ == 0) { status_ = Status::Failed; return; }
    status_ = Status::Sending;

    // Build AX.25 frame: dst APRS-0 (not last), src OUR-CALL (last),
    //   ctrl 0x03, pid 0xF0, info ":ADDRESSEE:msg"
    uint8_t frame[7 + 7 + 2 + 1 + 9 + 1 + kMsgCap + 4];
    size_t off = 0;
    // dst "APRS  "-0 not-last
    encode_addr_(frame, off, "APRS", 0, /*last=*/false);
    // src — split call_ into base+ssid
    char base[8] = {0};
    int  src_ssid = 0;
    parse_call_(call_, base, &src_ssid);
    encode_addr_(frame, off, base, src_ssid, /*last=*/true);
    frame[off++] = 0x03;
    frame[off++] = 0xF0;
    // info field: ":ADDRESSEE:msg" with addressee padded to 9
    frame[off++] = ':';
    char addr_pad[10];
    std::snprintf(addr_pad, sizeof(addr_pad), "%-9s", to_);
    for (int i = 0; i < 9; ++i) frame[off++] = static_cast<uint8_t>(addr_pad[i]);
    frame[off++] = ':';
    for (size_t i = 0; i < msg_len_; ++i) frame[off++] = static_cast<uint8_t>(msg_[i]);

    // KISS-wrap
    uint8_t kiss[2 * sizeof(frame) + 4];
    const size_t klen = kiss::encode_data(frame, off, kiss, sizeof(kiss));
    if (klen == 0) { status_ = Status::Failed; return; }
    status_ = radio_.kiss_write(kiss, klen) ? Status::Sent : Status::Failed;
  }

  static void encode_addr_(uint8_t* buf, size_t& off,
                           const char* call, int ssid, bool last) {
    for (int i = 0; i < 6; ++i) {
      const char c = (call[i] && call[i] != '\0') ? call[i] : ' ';
      buf[off++] = static_cast<uint8_t>(c) << 1;
    }
    // SSID byte: 0_11_<ssid>_<last>
    uint8_t b = 0x60 | static_cast<uint8_t>((ssid & 0x0F) << 1);
    if (last) b |= 0x01;
    buf[off++] = b;
  }

  static void parse_call_(const char* full, char* base_out, int* ssid_out) {
    *ssid_out = 0;
    const char* dash = std::strchr(full, '-');
    const size_t base_len = dash ? static_cast<size_t>(dash - full)
                                 : std::strlen(full);
    const size_t copy = (base_len < 7) ? base_len : 6;
    std::memcpy(base_out, full, copy);
    base_out[copy] = '\0';
    if (dash) *ssid_out = std::atoi(dash + 1);
  }

  IRadioLink& radio_;
  IStorage&   store_;
  Hal*        hal_   = nullptr;
  Field       field_ = Field::To;
  Status      status_ = Status::Idle;
  char        call_[12]    = {0};
  char        to_[kToCap]  = {0};
  size_t      to_len_      = 0;
  char        msg_[kMsgCap] = {0};
  size_t      msg_len_     = 0;
};

}  // namespace yui

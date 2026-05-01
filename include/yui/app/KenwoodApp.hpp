#pragma once
// KenwoodApp — admin/status view for the TH-D75 over IRadioLink. Shows
// connection state, last queried ID/model, current frequency + mode.
//
// Keys:
//   Enter  : send the next refresh batch (ID / FQ 0 / MD 0)
//   Tab    : (re)connect using the stored radio.mac (if present)
//   Esc    : back (handled by Shell)
//
// In v0.2 this is the entry point for the Radio category. Real impl
// of the BluetoothSerial-backed Esp32RadioLink lands later — until
// then this app shows "no radio" and Tab tries to connect (which
// stubs to false and reports Failed). All flows are exercised by
// FakeRadioLink in native tests.
#include "yui/app/App.hpp"
#include "yui/hal/IRadioLink.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class KenwoodApp : public App {
public:
  static constexpr size_t kBufSz = 32;

  KenwoodApp(IRadioLink& radio, IStorage* store = nullptr)
      : radio_(radio), store_(store) {}

  const char* name() const override { return "Kenwood"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& hal) override {
    hal_   = &hal;
    refresh_();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Enter) refresh_();
    if (k.key == Key::Tab)   try_connect_();
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "Kenwood TH-D75", kWhite, kJapanRed);

    char line[40];
    int y = 22;
    std::snprintf(line, sizeof(line), "Link: %s", state_label_());
    d.draw_text(8, y, line, kBlack, kWhite); y += 14;

    if (radio_.state() == RadioLinkState::CommandMode ||
        radio_.state() == RadioLinkState::KissMode) {
      std::snprintf(line, sizeof(line), "ID:   %s",  id_buf_[0]   ? id_buf_   : "—");
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
      std::snprintf(line, sizeof(line), "Freq: %s",  freq_buf_[0] ? freq_buf_ : "—");
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
      std::snprintf(line, sizeof(line), "Mode: %s",  mode_buf_[0] ? mode_buf_ : "—");
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
    } else {
      d.draw_text(8, y, "Tab to (re)connect", kJapanRedDark, kWhite); y += 14;
    }

    d.draw_text(8, d.height() - 14,
                "Enter:refresh Tab:connect Esc:back",
                kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  const char* id_text()   const { return id_buf_; }
  const char* freq_text() const { return freq_buf_; }
  const char* mode_text() const { return mode_buf_; }

private:
  void refresh_() {
    if (radio_.state() != RadioLinkState::CommandMode) return;
    radio_.command("ID", id_buf_,   sizeof(id_buf_),   1000);
    radio_.command("FQ 0", freq_buf_, sizeof(freq_buf_), 1000);
    radio_.command("MD 0", mode_buf_, sizeof(mode_buf_), 1000);
  }

  void try_connect_() {
    char mac[24] = {0};
    char pin[8]  = "0000";
    if (store_) {
      store_->get_str("radio.mac", mac, sizeof(mac));
      store_->get_str("radio.pin", pin, sizeof(pin));
      if (pin[0] == 0) std::strcpy(pin, "0000");
    }
    if (mac[0] == 0) return;   // nothing to try
    radio_.connect(mac, pin);
  }

  const char* state_label_() {
    switch (radio_.state()) {
      case RadioLinkState::Idle:        return "idle";
      case RadioLinkState::Connecting:  return "connecting";
      case RadioLinkState::CommandMode: return "ready";
      case RadioLinkState::KissMode:    return "KISS";
      case RadioLinkState::Failed:      return "failed";
    }
    return "?";
  }

  IRadioLink& radio_;
  IStorage*   store_ = nullptr;
  Hal*        hal_   = nullptr;
  char        id_buf_[kBufSz]   = {0};
  char        freq_buf_[kBufSz] = {0};
  char        mode_buf_[kBufSz] = {0};
};

}  // namespace yui

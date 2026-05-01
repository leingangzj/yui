#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32RadioLink — Bluetooth Classic SPP backend for the TH-D75.
//
// STUB: declared but not implemented. The actual BluetoothSerial wiring
// is v0.2 Track A code, written when hardware is in hand. Until then
// this class compiles into a no-op so main.cpp can construct one without
// a real BT connection.
//
// Per docs/protocols/KENWOOD_THD75.md:
// - Discovery filter: COD == 0x620204 (Kenwood handhelds)
// - Connect via BluetoothSerial::connect(mac, 0, ESP_SPP_SEC_NONE,
//   ESP_SPP_ROLE_MASTER) with setPin("0000")
// - Serial: 9600 8-N-1, ASCII commands \r-terminated
// - KISS entry: send "TN 2,0\r" (space mandatory)
// - KISS exit: send 0xC0 0xFF 0xC0
#include "yui/hal/IRadioLink.hpp"

namespace yui {

class Esp32RadioLink : public IRadioLink {
public:
  bool connect(const char* /*mac*/, const char* /*pin*/) override {
    state_ = RadioLinkState::Failed;
    return false;
  }
  void disconnect() override { state_ = RadioLinkState::Idle; }
  RadioLinkState state() override { return state_; }

  bool command(const char* /*cmd*/, char* /*out*/, size_t /*cap*/,
               uint32_t /*to*/) override { return false; }

  bool enter_kiss(uint8_t /*vfo*/) override { return false; }
  bool exit_kiss() override { return false; }
  int  kiss_read(uint8_t* /*buf*/, size_t /*cap*/) override { return -1; }
  bool kiss_write(const uint8_t* /*frame*/, size_t /*len*/) override { return false; }

private:
  RadioLinkState state_ = RadioLinkState::Idle;
};

}  // namespace yui
#endif

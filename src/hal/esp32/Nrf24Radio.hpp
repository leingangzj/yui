#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/INrf24.hpp"
#include "yui/config/pins.hpp"
#include "hal/esp32/HydraSpiBus.hpp"

#include <RadioLib.h>

namespace yui {

// nRF24L01+ 2.4 GHz driver using RadioLib. Lives on the Hydra cap;
// CS=6, CE (Pingequa labels io0)=4. Shares SPI with CC1101.
//
// Phase 3 scope: lifecycle (begin/probe/end), channel + data-rate +
// pipe-0 address, listen toggle, carrier-detect probe, packet
// transmit/receive, continuous carrier for jamming.
class Nrf24Radio : public INrf24 {
public:
  Nrf24Radio()
    : module_(pins::kNrf24Cs, RADIOLIB_NC, RADIOLIB_NC,
              pins::kNrf24Io0,            // CE goes in the gpio slot
              HydraSpiBus::instance()),
      radio_(&module_) {}

  bool begin() override {
    const int16_t st = radio_.begin();
    if (st != RADIOLIB_ERR_NONE) {
      last_error_ = "nRF24 begin() failed";
      present_ = false;
      return false;
    }
    present_ = true;
    return true;
  }

  bool is_present() override { return present_; }
  void end() override { radio_.standby(); present_ = false; }
  const char* last_error() const override { return last_error_; }

  bool set_channel(uint8_t ch) override {
    if (ch > 125) { last_error_ = "channel > 125"; return false; }
    // nRF24 frequency = 2400 + ch MHz
    const float mhz = 2400.0f + static_cast<float>(ch);
    const int16_t st = radio_.setFrequency(mhz);
    if (st != RADIOLIB_ERR_NONE) {
      last_error_ = "channel set failed";
      return false;
    }
    channel_ = ch;
    return true;
  }
  uint8_t channel() const override { return channel_; }

  bool set_data_rate(NrfDataRate r) override {
    int16_t st = RADIOLIB_ERR_NONE;
    switch (r) {
      case NrfDataRate::Rate250kbps: st = radio_.setBitRate(0.25f); break;
      case NrfDataRate::Rate1Mbps:   st = radio_.setBitRate(1.0f);  break;
      case NrfDataRate::Rate2Mbps:   st = radio_.setBitRate(2.0f);  break;
    }
    if (st != RADIOLIB_ERR_NONE) {
      last_error_ = "data rate set failed";
      return false;
    }
    rate_ = r;
    return true;
  }

  bool set_address(const uint8_t* addr, std::size_t len) override {
    if (len < 3 || len > 5) { last_error_ = "addr len 3..5"; return false; }
    const int16_t st = radio_.setAddressWidth(static_cast<uint8_t>(len));
    if (st != RADIOLIB_ERR_NONE) {
      last_error_ = "addr width";
      return false;
    }
    const int16_t st2 = radio_.setReceivePipe(0, addr);
    if (st2 != RADIOLIB_ERR_NONE) {
      last_error_ = "pipe addr";
      return false;
    }
    return true;
  }

  bool start_listening() override {
    const int16_t st = radio_.startReceive();
    listening_ = (st == RADIOLIB_ERR_NONE);
    return listening_;
  }
  bool stop_listening() override {
    radio_.standby();
    listening_ = false;
    return true;
  }

  bool carrier_detected() override {
    // RadioLib's nRF24 driver doesn't expose RPD/CD directly; fall back
    // to: are we receiving? A more accurate scanner reads register 0x09
    // via low-level SPI. Phase 3 stub returns false; Nrf24ScanApp in
    // Phase 4 will reach into low-level if scanning quality matters.
    return false;
  }

  int receive(uint8_t* buf, std::size_t max) override {
    size_t avail = 0;
    const int16_t st = radio_.readData(buf, max);
    if (st == RADIOLIB_ERR_NONE) {
      avail = radio_.getPacketLength(false);
      if (avail > max) avail = max;
      rx_total_ += avail;
      return static_cast<int>(avail);
    }
    return 0;
  }

  bool transmit(const uint8_t* data, std::size_t len) override {
    const int16_t st = radio_.transmit(data, len, 0);
    if (st != RADIOLIB_ERR_NONE) {
      last_error_ = "TX error";
      return false;
    }
    tx_total_ += len;
    return true;
  }

  bool set_carrier(bool on) override {
    if (on) {
      const int16_t st = radio_.transmitDirect();
      if (st != RADIOLIB_ERR_NONE) {
        last_error_ = "carrier on failed";
        return false;
      }
    } else {
      radio_.standby();
    }
    return true;
  }

  // Promiscuous mode (Bastille / Mousejack technique): the nRF24
  // doesn't natively support sniffing arbitrary addresses, but if we
  // configure address-width = 2 (a value the datasheet calls illegal)
  // and set RX_ADDR_P0 to a constant pattern, the chip's preamble +
  // CRC matcher still latches valid frames regardless of source
  // address. RadioLib doesn't expose this — we drop down to the
  // chip's SPI registers via Module's helpers.
  //
  //   SETUP_AW    (0x03) = 0x00   → 2-byte address (illegal)
  //   RX_ADDR_P0  (0x0A) = 0xAA, 0xAA  → preamble pattern
  //   EN_AA       (0x01) = 0x00   → no auto-ack
  //   EN_RXADDR   (0x02) = 0x01   → pipe 0 only
  //   RX_PW_P0    (0x11) = 32     → max payload
  //   CONFIG      (0x00) = 0x33   → PWR_UP=1, PRIM_RX=1, CRC=2 byte
  bool set_promiscuous(bool on) override {
    last_promiscuous_ack_ = false;
    if (on) {
      // SETUP_AW: bits 0-1 = address width. 0b00 = 'illegal' 2 bytes.
      module_.SPIwriteRegister(0x03, 0x00);
      const uint8_t addr[2] = {0xAA, 0xAA};
      module_.SPIwriteRegisterBurst(0x0A, addr, 2);
      module_.SPIwriteRegister(0x01, 0x00);   // disable auto-ack
      module_.SPIwriteRegister(0x02, 0x01);   // enable pipe 0
      module_.SPIwriteRegister(0x11, 0x20);   // 32-byte payload
      module_.SPIwriteRegister(0x00, 0x33);   // PWR_UP + PRIM_RX
      // F1 — read-back assertion: confirm SETUP_AW landed as 0x00.
      // If the SPI write silently failed (loose CS, dead chip), this
      // surfaces immediately rather than later as "scan finds nothing".
      const uint8_t aw = module_.SPIreadRegister(0x03);
      last_promiscuous_ack_ = (aw == 0x00);
      promiscuous_          = last_promiscuous_ack_;
      return last_promiscuous_ack_;
    }
    // Restore baseline so a subsequent begin() / set_address() works.
    module_.SPIwriteRegister(0x03, 0x03);
    const uint8_t aw = module_.SPIreadRegister(0x03);
    last_promiscuous_ack_ = (aw == 0x03);
    promiscuous_          = false;
    return last_promiscuous_ack_;
  }
  bool is_promiscuous() const override { return promiscuous_; }
  // Test seam: true iff the most recent set_promiscuous() read-back
  // confirmed the chip accepted the register write.
  bool last_promiscuous_ack() const { return last_promiscuous_ack_; }

  uint64_t rx_bytes() const override { return rx_total_; }
  uint64_t tx_bytes() const override { return tx_total_; }

private:
  Module module_;
  nRF24 radio_;
  bool present_ = false;
  const char* last_error_ = "";
  uint8_t channel_ = 76;
  NrfDataRate rate_ = NrfDataRate::Rate1Mbps;
  bool listening_ = false;
  bool promiscuous_ = false;
  bool last_promiscuous_ack_ = false;
  uint64_t rx_total_ = 0, tx_total_ = 0;
};

}  // namespace yui
#endif

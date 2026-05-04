#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/ICc1101.hpp"
#include "yui/config/pins.hpp"
#include "hal/esp32/HydraSpiBus.hpp"

#include <RadioLib.h>

namespace yui {

// CC1101 sub-GHz driver using RadioLib. The chip lives on the Hydra
// RF Cap 424; CS=13, GDO0=5, GDO2 unused. Shares SPI with NRF24.
//
// Phase 3 scope: lifecycle (begin/probe/end), tuning (frequency,
// modulation, bitrate), RSSI read, basic transmit/receive, and
// continuous-wave for jamming. Phase 4's apps fill in the higher-
// level protocol logic on top of these primitives.
class Cc1101Radio : public ICc1101 {
public:
  Cc1101Radio()
    : module_(pins::kCc1101Cs, pins::kCc1101Io0,
              RADIOLIB_NC, RADIOLIB_NC,
              HydraSpiBus::instance()),
      radio_(&module_) {}

  bool begin() override {
    // RadioLib's begin() defaults: 433.92 MHz, OOK, 4.8 kbps, 50 dB
    // bandwidth, +10 dBm output. Good baseline for 433 MHz remotes.
    const int16_t st = radio_.begin();
    if (st != RADIOLIB_ERR_NONE) {
      last_error_ = "CC1101 begin() failed";
      present_ = false;
      return false;
    }
    radio_.setOOK(true);
    present_ = true;
    return true;
  }

  bool is_present() override { return present_; }
  void end() override { radio_.standby(); present_ = false; }
  const char* last_error() const override { return last_error_; }

  bool set_frequency_hz(uint32_t hz) override {
    const float mhz = static_cast<float>(hz) / 1'000'000.0f;
    const int16_t st = radio_.setFrequency(mhz);
    if (st != RADIOLIB_ERR_NONE) {
      last_error_ = "freq out of band";
      return false;
    }
    freq_hz_ = hz;
    return true;
  }
  uint32_t frequency_hz() const override { return freq_hz_; }

  bool set_modulation(CcModulation m) override {
    int16_t st = RADIOLIB_ERR_NONE;
    switch (m) {
      case CcModulation::Ook:
      case CcModulation::Ask:
        st = radio_.setOOK(true);
        break;
      case CcModulation::Fsk2:
        st = radio_.setOOK(false);
        break;
      case CcModulation::Fsk4:
        // RadioLib CC1101 supports 2-FSK / GFSK / OOK / MSK; treat 4-FSK
        // as 2-FSK fallback for now (rare in the wild for our use cases).
        st = radio_.setOOK(false);
        break;
    }
    if (st != RADIOLIB_ERR_NONE) {
      last_error_ = "modulation set failed";
      return false;
    }
    mod_ = m;
    return true;
  }

  bool set_bitrate_bps(uint32_t bps) override {
    const float kbps = static_cast<float>(bps) / 1000.0f;
    const int16_t st = radio_.setBitRate(kbps);
    if (st != RADIOLIB_ERR_NONE) {
      last_error_ = "bitrate out of range";
      return false;
    }
    bps_ = bps;
    return true;
  }

  int16_t read_rssi_dbm() override {
    const float r = radio_.getRSSI();
    return static_cast<int16_t>(r);
  }

  int receive(uint8_t* buf, std::size_t max) override {
    size_t avail = 0;
    const int16_t st = radio_.readData(buf, max);
    if (st == RADIOLIB_ERR_RX_TIMEOUT || st == RADIOLIB_ERR_NONE) {
      avail = radio_.getPacketLength(false);
      if (avail > max) avail = max;
      rx_total_ += avail;
      return static_cast<int>(avail);
    }
    last_error_ = "RX error";
    return -1;
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
      // Direct-mode TX with no modulation = pure carrier.
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

  // Edge-toggle async TX: assert direct-mode TX, then bit-bang GDO0
  // by absolute-deadline timing so cumulative jitter doesn't drift.
  // The CC1101 in async mode uses GDO0 as the data input — TX-on
  // when high, TX-off when low. Timings are signed µs deltas
  // (positive = on, negative = off — Flipper RAW convention).
  bool transmit_raw_edges(const int32_t* timings_us,
                          std::size_t n) override {
    if (!timings_us || n == 0) return true;
    // Configure GDO0 (pins::kCc1101Io0) as a host output and assert
    // direct-mode TX so the modulator clocks the carrier off our pin.
    pinMode(pins::kCc1101Io0, OUTPUT);
    digitalWrite(pins::kCc1101Io0, LOW);
    const int16_t st = radio_.transmitDirect();
    if (st != RADIOLIB_ERR_NONE) {
      last_error_ = "raw TX direct-mode failed";
      return false;
    }
    // Walk the edges with absolute-deadline scheduling — each edge
    // ends at start + accumulated-µs, so cumulative jitter doesn't
    // drift the way "delay then toggle" does.
    const uint64_t t0 = static_cast<uint64_t>(::micros());
    uint64_t deadline = t0;
    for (std::size_t i = 0; i < n; ++i) {
      const int32_t t = timings_us[i];
      const bool on = t > 0;
      const uint32_t dur = static_cast<uint32_t>(on ? t : -t);
      digitalWrite(pins::kCc1101Io0, on ? HIGH : LOW);
      deadline += dur;
      // Spin until we hit the deadline. delayMicroseconds rounds
      // down; this gives more accurate edges for small dt.
      while (static_cast<uint64_t>(::micros()) < deadline) { /* spin */ }
    }
    digitalWrite(pins::kCc1101Io0, LOW);
    radio_.standby();
    tx_total_ += n;
    return true;
  }

  uint64_t rx_bytes() const override { return rx_total_; }
  uint64_t tx_bytes() const override { return tx_total_; }

private:
  Module module_;
  CC1101 radio_;
  bool present_ = false;
  const char* last_error_ = "";
  uint32_t freq_hz_ = 433'920'000;
  CcModulation mod_ = CcModulation::Ook;
  uint32_t bps_ = 4800;
  uint64_t rx_total_ = 0, tx_total_ = 0;
};

}  // namespace yui
#endif

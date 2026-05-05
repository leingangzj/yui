#pragma once
// NativeNimBleRawTx / NativeNrf24BleRawTx — host-side fakes for the
// Phase 5.2 dual-rail BLE TX engines. No real radios; tests assert on
// channel assignment, tx_count(), and start/stop transitions.
#include "yui/hal/IBleRawTx.hpp"
#include "yui/hal/INrf24.hpp"
#include <cstring>
#include <vector>

namespace yui {

// NimBLE-rail fake. Pretends to live on the S3's BLE radio.
class FakeNimBleRawTx : public IBleRawTx {
 public:
  void set_present(bool p) { present_ = p; }
  bool        is_present() const override { return present_; }
  const char* rail_name()  const override { return "NimBLE"; }
  bool set_channel(int ch) override {
    if (ch < kChannel37 || ch > kChannel39) return false;
    ch_ = ch;
    return true;
  }
  int  channel() const override { return ch_; }
  bool tx_advert(const uint8_t* payload, std::size_t len) override {
    if (!present_ || !payload || len == 0 || len > 31) return false;
    last_payload_.assign(payload, payload + len);
    ++tx_count_;
    return true;
  }
  bool start_continuous(int ch) override {
    if (!set_channel(ch)) return false;
    active_ = true;
    return true;
  }
  void stop() override                 { active_ = false; }
  bool is_active() const override      { return active_; }
  std::size_t tx_count() const override { return tx_count_; }

  const std::vector<uint8_t>& last_payload() const { return last_payload_; }

 private:
  bool        present_  = true;
  int         ch_       = kChannel37;
  bool        active_   = false;
  std::size_t tx_count_ = 0;
  std::vector<uint8_t> last_payload_;
};

// nRF24-rail fake. Models the channel-mapping (BLE 37/38/39 → nRF24
// 2/26/80) so tests can assert correct mapping.
class FakeNrf24BleRawTx : public IBleRawTx {
 public:
  void set_present(bool p) { present_ = p; }
  bool        is_present() const override { return present_; }
  const char* rail_name()  const override { return "nRF24"; }

  static int adv_channel_to_nrf24_channel(int adv_ch) {
    switch (adv_ch) {
      case kChannel37: return 2;   // 2402 MHz
      case kChannel38: return 26;  // 2426 MHz
      case kChannel39: return 80;  // 2480 MHz
    }
    return -1;
  }

  bool set_channel(int ch) override {
    int n = adv_channel_to_nrf24_channel(ch);
    if (n < 0) return false;
    ch_ = ch;
    nrf_ch_ = n;
    return true;
  }
  int  channel()        const override { return ch_; }
  int  nrf_channel()    const         { return nrf_ch_; }

  bool tx_advert(const uint8_t* payload, std::size_t len) override {
    if (!present_ || !payload || len == 0 || len > 31) return false;
    last_payload_.assign(payload, payload + len);
    ++tx_count_;
    return true;
  }
  bool start_continuous(int ch) override {
    if (!set_channel(ch)) return false;
    active_ = true;
    return true;
  }
  void stop() override                 { active_ = false; }
  bool is_active() const override      { return active_; }
  std::size_t tx_count() const override { return tx_count_; }

  const std::vector<uint8_t>& last_payload() const { return last_payload_; }

 private:
  bool        present_  = true;
  int         ch_       = kChannel37;
  int         nrf_ch_   = 2;
  bool        active_   = false;
  std::size_t tx_count_ = 0;
  std::vector<uint8_t> last_payload_;
};

}  // namespace yui

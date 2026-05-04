#pragma once
#include "yui/hal/ICc1101.hpp"
#include <cstring>
#include <vector>

namespace yui {

// Host-side fake CC1101 — lets tests assert that apps drive the radio
// correctly (frequency hops, transmit calls, RSSI reads) without any
// real hardware.
//
// Default: present_ = true so apps under test see the chip. Tests
// flip it off to validate "cap not detected" behaviour.
class NativeCc1101 : public ICc1101 {
public:
  bool begin() override { return present_; }
  bool is_present() override { return present_; }
  const char* last_error() const override { return last_error_; }

  bool set_frequency_hz(uint32_t hz) override {
    if (hz < 300'000'000 || hz > 928'000'000) {
      last_error_ = "freq out of band";
      return false;
    }
    freq_hz_ = hz;
    return true;
  }
  uint32_t frequency_hz() const override { return freq_hz_; }

  bool set_modulation(CcModulation m) override { mod_ = m; return true; }
  bool set_bitrate_bps(uint32_t bps) override { bps_ = bps; return true; }

  int16_t read_rssi_dbm() override { return canned_rssi_; }

  int receive(uint8_t* buf, std::size_t max) override {
    if (rx_queue_.empty()) return 0;
    const auto& pkt = rx_queue_.front();
    const std::size_t n = pkt.size() < max ? pkt.size() : max;
    std::memcpy(buf, pkt.data(), n);
    rx_queue_.erase(rx_queue_.begin());
    rx_total_ += n;
    return static_cast<int>(n);
  }

  bool transmit(const uint8_t* data, std::size_t len) override {
    last_tx_.assign(data, data + len);
    tx_total_ += len;
    ++tx_calls_;
    return true;
  }

  bool set_carrier(bool on) override { carrier_on_ = on; return true; }

  // Native edge-toggle: record the timing sequence as-is so tests
  // can verify order + count + sign without modeling real GPIO.
  bool transmit_raw_edges(const int32_t* timings_us,
                          std::size_t n) override {
    last_edges_.assign(timings_us, timings_us + n);
    tx_total_ += n * 4;  // count edge-bytes for telemetry symmetry
    ++tx_calls_;
    return true;
  }
  const std::vector<int32_t>& last_edges() const { return last_edges_; }

  uint64_t rx_bytes() const override { return rx_total_; }
  uint64_t tx_bytes() const override { return tx_total_; }

  // ── Test scaffolding ────────────────────────────────────────────
  void set_present(bool p) { present_ = p; }
  void set_canned_rssi(int16_t db) { canned_rssi_ = db; }
  void enqueue_rx(const std::vector<uint8_t>& pkt) { rx_queue_.push_back(pkt); }
  const std::vector<uint8_t>& last_tx() const { return last_tx_; }
  uint32_t tx_call_count() const { return tx_calls_; }
  bool carrier_on() const { return carrier_on_; }
  CcModulation modulation() const { return mod_; }
  uint32_t bitrate_bps() const { return bps_; }

private:
  bool present_ = true;
  const char* last_error_ = "";
  uint32_t freq_hz_ = 433'920'000;
  CcModulation mod_ = CcModulation::Ook;
  uint32_t bps_ = 4800;
  int16_t canned_rssi_ = -80;
  std::vector<std::vector<uint8_t>> rx_queue_;
  std::vector<uint8_t> last_tx_;
  std::vector<int32_t> last_edges_;
  uint64_t rx_total_ = 0, tx_total_ = 0;
  uint32_t tx_calls_ = 0;
  bool carrier_on_ = false;
};

}  // namespace yui

#pragma once
#include "yui/hal/INrf24.hpp"
#include <cstring>
#include <vector>

namespace yui {

// Host-side fake nRF24L01+ — same shape as NativeCc1101 but for the
// 2.4 GHz radio.
class NativeNrf24 : public INrf24 {
public:
  bool begin() override { return present_; }
  bool is_present() override { return present_; }
  const char* last_error() const override { return last_error_; }

  bool set_channel(uint8_t ch) override {
    if (ch > 125) { last_error_ = "channel > 125"; return false; }
    channel_ = ch;
    return true;
  }
  uint8_t channel() const override { return channel_; }

  bool set_data_rate(NrfDataRate r) override { rate_ = r; return true; }

  bool set_address(const uint8_t* addr, std::size_t len) override {
    if (len < 3 || len > 5) { last_error_ = "addr len"; return false; }
    addr_.assign(addr, addr + len);
    return true;
  }

  bool start_listening() override { listening_ = true; return true; }
  bool stop_listening() override { listening_ = false; return true; }
  bool carrier_detected() override { return canned_carrier_; }

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

  // Native promiscuous-mode tracking: tests can assert the app
  // turned it on before scanning.
  bool set_promiscuous(bool on) override { promiscuous_ = on; return true; }
  bool is_promiscuous() const override { return promiscuous_; }

  uint64_t rx_bytes() const override { return rx_total_; }
  uint64_t tx_bytes() const override { return tx_total_; }

  // ── Test scaffolding ────────────────────────────────────────────
  void set_present(bool p) { present_ = p; }
  void set_canned_carrier(bool on) { canned_carrier_ = on; }
  void enqueue_rx(const std::vector<uint8_t>& pkt) { rx_queue_.push_back(pkt); }
  const std::vector<uint8_t>& last_tx() const { return last_tx_; }
  uint32_t tx_call_count() const { return tx_calls_; }
  bool carrier_on() const { return carrier_on_; }
  bool listening() const { return listening_; }
  NrfDataRate data_rate() const { return rate_; }
  const std::vector<uint8_t>& address() const { return addr_; }

private:
  bool present_ = true;
  const char* last_error_ = "";
  uint8_t channel_ = 76;  // common default for nRF24
  NrfDataRate rate_ = NrfDataRate::Rate1Mbps;
  std::vector<uint8_t> addr_;
  bool listening_ = false;
  bool canned_carrier_ = false;
  std::vector<std::vector<uint8_t>> rx_queue_;
  std::vector<uint8_t> last_tx_;
  uint64_t rx_total_ = 0, tx_total_ = 0;
  uint32_t tx_calls_ = 0;
  bool carrier_on_ = false;
  bool promiscuous_ = false;
};

}  // namespace yui

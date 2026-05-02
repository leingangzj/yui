#pragma once
#include "yui/hal/IWifiMonitor.hpp"
#include <vector>

namespace yui {

// FakeWifiMonitor — tests inject frames synchronously via inject_frame().
class FakeWifiMonitor : public IWifiMonitor {
public:
  bool start(const WifiFilter& filter, uint8_t channel) override {
    filter_ = filter;
    channel_ = channel;
    running_ = true;
    return true;
  }
  void stop() override { running_ = false; }
  bool set_channel(uint8_t ch) override { channel_ = ch; return running_; }
  void set_callback(WifiRxCallback cb, void* ctx) override {
    cb_ = cb; ctx_ = ctx;
  }
  bool running() const override { return running_; }
  uint8_t channel() const override { return channel_; }

  // Test helper: deliver a frame to the registered callback.
  void inject_frame(const uint8_t* frame, size_t len, const WifiRxMeta& meta) {
    if (!running_ || !cb_) return;
    if (meta.type == WifiPktType::Management && !filter_.mgmt) return;
    if (meta.type == WifiPktType::Control    && !filter_.ctrl) return;
    if (meta.type == WifiPktType::Data       && !filter_.data) return;
    if (meta.type == WifiPktType::Misc       && !filter_.misc) return;
    cb_(ctx_, frame, len, meta);
  }

  // Test helper: simulate a channel hop sweep (1..13).
  void simulate_hop(uint8_t to) { channel_ = to; }

  bool tx_raw(const uint8_t* frame, size_t len) override {
    if (!running_) return false;
    tx_log_.emplace_back(frame, frame + len);
    return true;
  }

  const WifiFilter& filter() const { return filter_; }
  const std::vector<std::vector<uint8_t>>& tx_log() const { return tx_log_; }
  size_t tx_count() const { return tx_log_.size(); }

private:
  WifiFilter      filter_{};
  WifiRxCallback  cb_      = nullptr;
  void*           ctx_     = nullptr;
  uint8_t         channel_ = 0;
  bool            running_ = false;
  std::vector<std::vector<uint8_t>> tx_log_;
};

}  // namespace yui

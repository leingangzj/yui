#pragma once
#include "yui/hal/IRadioLink.hpp"
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace yui {

// FakeRadioLink — drop-in test backend. Tests register canned command
// responses ahead of time; KISS RX is fed via a queue of frames.
class FakeRadioLink : public IRadioLink {
public:
  // Test fixture: register an expected reply for a given ASCII command.
  // The reply is returned without the trailing \r — impl appends it.
  void register_reply(const char* cmd, const char* reply) {
    replies_[cmd ? cmd : ""] = reply ? reply : "";
  }

  // Test fixture: queue a KISS-mode read result (raw bytes, no framing
  // assumed — tests simulate exactly what the radio would deliver).
  void queue_kiss_bytes(const uint8_t* bytes, size_t len) {
    kiss_rx_.insert(kiss_rx_.end(), bytes, bytes + len);
  }

  bool connect(const char* mac, const char* /*pin*/) override {
    last_mac_ = mac ? mac : "";
    state_    = connect_immediate_ ? RadioLinkState::CommandMode
                                   : RadioLinkState::Connecting;
    ++connects_;
    return true;
  }
  void disconnect() override {
    state_ = RadioLinkState::Idle;
    kiss_tx_.clear();
    kiss_rx_.clear();
  }
  RadioLinkState state() override { return state_; }

  bool command(const char* cmd, char* out, size_t cap, uint32_t /*to*/) override {
    if (state_ != RadioLinkState::CommandMode) return false;
    last_cmd_ = cmd ? cmd : "";
    auto it = replies_.find(last_cmd_);
    if (it == replies_.end()) return false;
    if (cap == 0) return true;
    const size_t n = std::min(cap - 1, it->second.size());
    std::memcpy(out, it->second.data(), n);
    out[n] = '\0';
    return true;
  }

  bool enter_kiss(uint8_t /*vfo*/) override {
    if (state_ != RadioLinkState::CommandMode) return false;
    state_ = RadioLinkState::KissMode;
    return true;
  }
  bool exit_kiss() override {
    if (state_ != RadioLinkState::KissMode) return false;
    state_ = RadioLinkState::CommandMode;
    return true;
  }

  int kiss_read(uint8_t* buf, size_t cap) override {
    if (state_ != RadioLinkState::KissMode) return -1;
    if (kiss_rx_.empty() || cap == 0) return 0;
    const size_t n = std::min(cap, kiss_rx_.size());
    std::memcpy(buf, kiss_rx_.data(), n);
    kiss_rx_.erase(kiss_rx_.begin(), kiss_rx_.begin() + n);
    return static_cast<int>(n);
  }
  bool kiss_write(const uint8_t* frame, size_t len) override {
    if (state_ != RadioLinkState::KissMode) return false;
    kiss_tx_.insert(kiss_tx_.end(), frame, frame + len);
    return true;
  }

  // Test inspection
  void simulate_connected() { state_ = RadioLinkState::CommandMode; }
  void simulate_failed()    { state_ = RadioLinkState::Failed; }
  void set_connect_immediate(bool v) { connect_immediate_ = v; }
  int  connects() const { return connects_; }
  const std::string& last_mac() const { return last_mac_; }
  const std::string& last_cmd() const { return last_cmd_; }
  const std::vector<uint8_t>& kiss_tx() const { return kiss_tx_; }

private:
  RadioLinkState state_ = RadioLinkState::Idle;
  std::map<std::string, std::string> replies_;
  std::vector<uint8_t> kiss_rx_;
  std::vector<uint8_t> kiss_tx_;
  std::string last_mac_;
  std::string last_cmd_;
  bool connect_immediate_ = true;
  int  connects_ = 0;
};

}  // namespace yui

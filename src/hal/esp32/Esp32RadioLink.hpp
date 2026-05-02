#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32RadioLink — BLE central that connects to a "B.B. Link" bridge
// (islandmagic/bb-link), which in turn relays bytes to/from a Kenwood
// TH-D75 over Bluetooth Classic SPP. The Cardputer ADV's ESP32-S3 is
// BLE-only, so the bridge is mandatory; see docs/protocols/KENWOOD_THD75.md
// for the full architectural rationale.
//
// bb-link BLE service:
//   service:  00000001-ba2a-46c9-ae49-01b0961f68bb
//   TX char:  00000002-ba2a-46c9-ae49-01b0961f68bb  (write w/o response)
//   RX char:  00000003-ba2a-46c9-ae49-01b0961f68bb  (notify)
//   adv name: "B.B. Link"
//
// connect() argument is the bridge's BLE MAC (or address), not the
// radio's BT-Classic MAC. Pairing the bridge to the radio is done
// once in bb-link's own setup flow.
#include "yui/hal/IRadioLink.hpp"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>

namespace yui {

class Esp32RadioLink : public IRadioLink {
public:
  static constexpr const char* kAdvName       = "B.B. Link";
  static constexpr const char* kServiceUuid   = "00000001-ba2a-46c9-ae49-01b0961f68bb";
  static constexpr const char* kTxCharUuid    = "00000002-ba2a-46c9-ae49-01b0961f68bb";
  static constexpr const char* kRxCharUuid    = "00000003-ba2a-46c9-ae49-01b0961f68bb";

  static Esp32RadioLink* instance() { return self_; }

  Esp32RadioLink() { self_ = this; }
  ~Esp32RadioLink() override { if (self_ == this) self_ = nullptr; }

  bool connect(const char* mac_or_empty, const char* /*pin*/) override {
    if (!ble_inited_) {
      BLEDevice::init("Yui");
      ble_inited_ = true;
    }
    // Drop any bytes that survived a prior session before opening the
    // notify gate again — the static notify_cb_ uses state_ to decide
    // whether to push, so transition through Connecting after the flush.
    flush_rx_();
    state_ = RadioLinkState::Connecting;

    BLEAddress* target_addr = nullptr;
    if (mac_or_empty && mac_or_empty[0]) {
      target_addr = new BLEAddress(mac_or_empty);
    } else {
      // Scan for advertised name match.
      BLEScan* scan = BLEDevice::getScan();
      scan->setActiveScan(true);
      scan->setInterval(100);
      scan->setWindow(99);
      BLEScanResults res = scan->start(5, false);
      for (int i = 0; i < res.getCount(); ++i) {
        BLEAdvertisedDevice d = res.getDevice(i);
        if (d.haveName() && d.getName() == kAdvName) {
          target_addr = new BLEAddress(d.getAddress());
          break;
        }
      }
      scan->clearResults();
    }
    if (!target_addr) { state_ = RadioLinkState::Failed; return false; }

    client_ = BLEDevice::createClient();
    if (!client_->connect(*target_addr)) {
      state_ = RadioLinkState::Failed;
      delete target_addr;
      return false;
    }
    delete target_addr;

    BLEUUID svc_uuid(kServiceUuid);
    BLERemoteService* svc = client_->getService(svc_uuid);
    if (!svc) {
      client_->disconnect();
      state_ = RadioLinkState::Failed;
      return false;
    }
    tx_char_ = svc->getCharacteristic(BLEUUID(kTxCharUuid));
    rx_char_ = svc->getCharacteristic(BLEUUID(kRxCharUuid));
    if (!tx_char_ || !rx_char_) {
      client_->disconnect();
      state_ = RadioLinkState::Failed;
      return false;
    }
    if (rx_char_->canNotify()) {
      rx_char_->registerForNotify(&Esp32RadioLink::notify_cb_);
    }

    state_   = RadioLinkState::CommandMode;
    in_kiss_ = false;
    return true;
  }

  void disconnect() override {
    // Flip state first so any in-flight BLE notify callbacks are dropped
    // by notify_cb_ rather than landing in rx_buf_ to be carried into
    // the next connect() cycle.
    state_   = RadioLinkState::Idle;
    in_kiss_ = false;
    if (client_ && client_->isConnected()) client_->disconnect();
    flush_rx_();
  }

  RadioLinkState state() override {
    if (state_ == RadioLinkState::Idle ||
        state_ == RadioLinkState::Failed) return state_;
    if (!client_ || !client_->isConnected()) {
      state_   = RadioLinkState::Idle;
      in_kiss_ = false;
    }
    return state_;
  }

  bool command(const char* cmd, char* out, size_t cap,
               uint32_t timeout_ms) override {
    if (state_ != RadioLinkState::CommandMode || !cmd || !tx_char_) return false;
    flush_rx_();
    write_str_(cmd);
    write_str_("\r");
    return read_until_cr_(out, cap, timeout_ms);
  }

  bool enter_kiss(uint8_t vfo) override {
    if (state_ != RadioLinkState::CommandMode) return false;
    char cmd[12];
    std::snprintf(cmd, sizeof(cmd), "TN 2,%u\r", static_cast<unsigned>(vfo));
    write_str_(cmd);
    delay(150);
    in_kiss_ = true;
    state_   = RadioLinkState::KissMode;
    return true;
  }

  bool exit_kiss() override {
    if (state_ != RadioLinkState::KissMode) return false;
    const uint8_t exit_frame[3] = {0xC0, 0xFF, 0xC0};
    write_bytes_(exit_frame, sizeof(exit_frame));
    in_kiss_ = false;
    state_   = RadioLinkState::CommandMode;
    return true;
  }

  int kiss_read(uint8_t* buf, size_t cap) override {
    if (!client_ || !client_->isConnected()) return -1;
    if (cap == 0) return 0;
    int n = 0;
    while (!rx_buf_.empty() && static_cast<size_t>(n) < cap) {
      const uint8_t b = rx_buf_.front();
      rx_buf_.erase(rx_buf_.begin());
      if (in_kiss_) {
        buf[n++] = b;
      } else {
        nmea_feed_(static_cast<char>(b));
      }
    }
    return n;
  }

  bool kiss_write(const uint8_t* frame, size_t len) override {
    if (state_ != RadioLinkState::KissMode || !frame || !tx_char_) return false;
    write_bytes_(frame, len);
    return true;
  }

  // Bridge for Esp32Gnss — pump RX buffer and surface NMEA lines.
  bool peek_nmea_line(char* out, size_t cap) {
    if (in_kiss_ || !client_ || !client_->isConnected()) return false;
    while (!rx_buf_.empty()) {
      nmea_feed_(static_cast<char>(rx_buf_.front()));
      rx_buf_.erase(rx_buf_.begin());
    }
    if (!have_line_ || cap == 0) return false;
    const size_t n = (line_len_ < cap - 1) ? line_len_ : cap - 1;
    std::memcpy(out, line_buf_, n);
    out[n] = '\0';
    have_line_ = false;
    return true;
  }

private:
  void write_str_(const char* s) { write_bytes_(reinterpret_cast<const uint8_t*>(s), std::strlen(s)); }

  void write_bytes_(const uint8_t* b, size_t n) {
    if (!tx_char_) return;
    // Chunk to MTU-3 (default 20 bytes) — bb-link handles reassembly.
    const size_t chunk = 20;
    for (size_t off = 0; off < n; off += chunk) {
      const size_t take = (n - off < chunk) ? (n - off) : chunk;
      tx_char_->writeValue(const_cast<uint8_t*>(b + off), take, false);
    }
  }

  void flush_rx_() { rx_buf_.clear(); have_line_ = false; line_len_ = 0; }

  bool read_until_cr_(char* out, size_t cap, uint32_t timeout_ms) {
    if (!out || cap == 0) return false;
    out[0] = '\0';
    const uint32_t deadline = millis() + timeout_ms;
    size_t n = 0;
    while (millis() < deadline) {
      while (!rx_buf_.empty() && n + 1 < cap) {
        const uint8_t b = rx_buf_.front();
        rx_buf_.erase(rx_buf_.begin());
        if (b == '\r') { out[n] = '\0'; return n > 0; }
        out[n++] = static_cast<char>(b);
      }
      delay(2);
    }
    out[n] = '\0';
    return false;
  }

  void nmea_feed_(char c) {
    if (c == '\r') return;
    if (c == '\n') {
      if (line_len_ > 0) {
        line_buf_[line_len_] = '\0';
        have_line_ = true;
      }
      line_len_ = 0;
      return;
    }
    if (line_len_ + 1 < sizeof(line_buf_)) line_buf_[line_len_++] = c;
    else                                    line_len_ = 0;
  }

  static void notify_cb_(BLERemoteCharacteristic* /*c*/, uint8_t* data,
                         size_t len, bool /*is_notify*/) {
    if (!self_) return;
    // Drop late notifies after disconnect — otherwise they'd corrupt
    // the next session's rx_buf_.
    const RadioLinkState s = self_->state_;
    if (s == RadioLinkState::Idle || s == RadioLinkState::Failed) return;
    for (size_t i = 0; i < len; ++i) self_->rx_buf_.push_back(data[i]);
  }

  static Esp32RadioLink*  self_;
  bool                    ble_inited_ = false;
  BLEClient*              client_     = nullptr;
  BLERemoteCharacteristic* tx_char_   = nullptr;
  BLERemoteCharacteristic* rx_char_   = nullptr;
  std::vector<uint8_t>    rx_buf_;
  RadioLinkState          state_      = RadioLinkState::Idle;
  bool                    in_kiss_    = false;
  char                    line_buf_[120] = {0};
  size_t                  line_len_   = 0;
  bool                    have_line_  = false;
};

inline Esp32RadioLink* Esp32RadioLink::self_ = nullptr;

}  // namespace yui
#endif

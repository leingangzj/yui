#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)
// Esp32BleAdvertiser — wraps the ESP32 Arduino BLE library's
// BLEAdvertising. set_payload() rebuilds the raw adv data each call;
// enable() starts/restarts advertising; disable() stops.
//
// Note: BLEDevice::init() can be called multiple times safely (idempotent).
// We share the same NimBLE/Bluedroid stack with Esp32RadioLink and
// Esp32BleCentral — coexistence is fine for advertising + scanning,
// but you can't be both peripheral and central at the exact same
// instant. BleSpamApp/BleJammerApp toggle this on; the GATT/RadioLink
// apps are central role and don't conflict at the API level.
#include "yui/hal/IBleAdvertiser.hpp"
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <cstring>

namespace yui {

class Esp32BleAdvertiser : public IBleAdvertiser {
public:
  bool set_payload(const uint8_t* p, size_t len) override {
    if (!p || len == 0 || len > 31) return false;
    payload_len_ = len;
    std::memcpy(payload_, p, len);
    if (active_ && adv_) restart_with_current_payload_();
    return true;
  }

  bool enable() override {
    if (!ensure_inited_()) return false;
    if (!adv_) adv_ = BLEDevice::getAdvertising();
    if (!adv_) return false;
    apply_payload_to_(adv_);
    adv_->start();
    active_ = true;
    return true;
  }

  void disable() override {
    if (adv_) adv_->stop();
    active_ = false;
  }

  bool active() const override { return active_; }

private:
  bool ensure_inited_() {
    if (!ble_inited_) {
      BLEDevice::init("Yui");
      ble_inited_ = true;
    }
    return true;
  }

  void apply_payload_to_(BLEAdvertising* a) {
    BLEAdvertisementData data;
    std::string raw(reinterpret_cast<const char*>(payload_), payload_len_);
    data.addData(raw);
    a->setAdvertisementData(data);
  }

  void restart_with_current_payload_() {
    adv_->stop();
    apply_payload_to_(adv_);
    adv_->start();
  }

  bool             ble_inited_ = false;
  BLEAdvertising*  adv_        = nullptr;
  uint8_t          payload_[31] = {0};
  size_t           payload_len_ = 0;
  bool             active_     = false;
};

}  // namespace yui
#endif

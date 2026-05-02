#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32BleCentral — wraps the Arduino BLEClient. Connects by MAC,
// walks services + characteristics, reads values. Used by BleGattApp.
//
// Shares BLEDevice initialization with Esp32RadioLink and
// Esp32BleAdvertiser; init() is idempotent.
#include "yui/hal/IBleCentral.hpp"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAddress.h>
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>
#include <cstring>

namespace yui {

class Esp32BleCentral : public IBleCentral {
public:
  bool connect(const char* mac) override {
    if (!mac) return false;
    if (!ble_inited_) { BLEDevice::init("Yui"); ble_inited_ = true; }
    if (client_ && client_->isConnected()) client_->disconnect();
    client_ = BLEDevice::createClient();
    BLEAddress addr(mac);
    if (!client_->connect(addr)) return false;
    return true;
  }

  void disconnect() override {
    if (client_ && client_->isConnected()) client_->disconnect();
  }

  bool connected() const override {
    return client_ && client_->isConnected();
  }

  size_t enumerate_services(GattService* out, size_t cap) override {
    if (!connected() || !out) return 0;
    auto* svcs = client_->getServices();
    if (!svcs) return 0;
    size_t n = 0;
    for (auto& kv : *svcs) {
      if (n >= cap) break;
      const std::string s = kv.first;
      std::strncpy(out[n].uuid, s.c_str(), sizeof(out[n].uuid) - 1);
      out[n].uuid[sizeof(out[n].uuid) - 1] = '\0';
      ++n;
    }
    return n;
  }

  size_t enumerate_characteristics(const char* service_uuid,
                                   GattCharacteristic* out,
                                   size_t cap) override {
    if (!connected() || !service_uuid || !out) return 0;
    BLERemoteService* svc = client_->getService(BLEUUID(service_uuid));
    if (!svc) return 0;
    auto* chars = svc->getCharacteristics();
    if (!chars) return 0;
    size_t n = 0;
    for (auto& kv : *chars) {
      if (n >= cap) break;
      std::strncpy(out[n].uuid, kv.first.c_str(), sizeof(out[n].uuid) - 1);
      out[n].uuid[sizeof(out[n].uuid) - 1] = '\0';
      uint8_t props = 0;
      if (kv.second->canRead())     props |= 0x02;
      if (kv.second->canWrite())    props |= 0x08;
      if (kv.second->canNotify())   props |= 0x10;
      if (kv.second->canIndicate()) props |= 0x20;
      out[n].properties = props;
      ++n;
    }
    return n;
  }

  int read_characteristic(const char* service_uuid, const char* char_uuid,
                          uint8_t* out, size_t cap) override {
    if (!connected() || !service_uuid || !char_uuid || !out || cap == 0) return -1;
    BLERemoteService* svc = client_->getService(BLEUUID(service_uuid));
    if (!svc) return -1;
    BLERemoteCharacteristic* c = svc->getCharacteristic(BLEUUID(char_uuid));
    if (!c || !c->canRead()) return -1;
    std::string v = c->readValue();
    const size_t n = (v.size() < cap) ? v.size() : cap;
    std::memcpy(out, v.data(), n);
    return static_cast<int>(n);
  }

private:
  bool       ble_inited_ = false;
  BLEClient* client_     = nullptr;
};

}  // namespace yui
#endif

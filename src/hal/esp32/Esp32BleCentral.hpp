#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32BleCentral — wraps NimBLE BLEClient. STUB until v0.3 hardware
// bring-up. Real impl uses BLEDevice::createClient(); client->connect();
// client->getService(); service->getCharacteristics().
#include "yui/hal/IBleCentral.hpp"

namespace yui {

class Esp32BleCentral : public IBleCentral {
public:
  bool connect(const char* /*mac*/) override { return false; }
  void disconnect() override { connected_ = false; }
  bool connected() const override { return connected_; }
  size_t enumerate_services(GattService*, size_t) override { return 0; }
  size_t enumerate_characteristics(const char*, GattCharacteristic*, size_t) override { return 0; }
  int read_characteristic(const char*, const char*, uint8_t*, size_t) override { return -1; }
private:
  bool connected_ = false;
};

}  // namespace yui
#endif

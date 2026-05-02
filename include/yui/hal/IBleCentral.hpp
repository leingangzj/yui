#pragma once
// IBleCentral — connect to a peripheral and enumerate its GATT
// services + characteristics. Used by BleGattApp.
//
// Esp32BleCentral wraps NimBLE (BLEDevice::createClient + connect +
// getService + getCharacteristics). Native fake answers from a
// pre-registered service tree.
#include <cstdint>
#include <cstddef>

namespace yui {

struct GattService {
  char     uuid[40] = {0};   // string-form 16/32/128-bit UUID
};

struct GattCharacteristic {
  char    uuid[40]   = {0};
  uint8_t properties = 0;    // BLE props bitmap (0x02=read, 0x08=write, 0x10=notify, ...)
};

class IBleCentral {
public:
  virtual ~IBleCentral() = default;

  // Connect to peripheral by MAC string.
  virtual bool connect(const char* mac) = 0;
  virtual void disconnect() = 0;
  virtual bool connected() const = 0;

  // Discover services on the connected peripheral. Returns count.
  virtual size_t enumerate_services(GattService* out, size_t cap) = 0;

  // Discover characteristics under a particular service UUID.
  virtual size_t enumerate_characteristics(const char* service_uuid,
                                           GattCharacteristic* out,
                                           size_t cap) = 0;

  // Read a characteristic value as raw bytes; returns bytes copied.
  virtual int read_characteristic(const char* service_uuid,
                                  const char* char_uuid,
                                  uint8_t* out, size_t cap) = 0;
};

}  // namespace yui

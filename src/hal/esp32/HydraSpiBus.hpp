#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/config/pins.hpp"
#include <SPI.h>

namespace yui {

// Shared SPI bus for the Pingequa Hydra RF Cap 424.
//
// Both CC1101 and nRF24L01+ live on the same SCK/MOSI/MISO trio
// (= GPIO 40 / 14 / 39). Only the CS pin differs per chip. Auto-
// switching of the bus between the two radios happens on the cap; the
// host just toggles each chip's CS.
//
// We expose a single SPIClass instance that both Cc1101Radio and
// Nrf24Radio share. They each take it by reference.
class HydraSpiBus {
public:
  static SPIClass& instance() {
    static SPIClass bus(HSPI);
    static bool inited = false;
    if (!inited) {
      bus.begin(pins::kHydraSck, pins::kHydraMiso, pins::kHydraMosi);
      inited = true;
    }
    return bus;
  }
};

}  // namespace yui
#endif

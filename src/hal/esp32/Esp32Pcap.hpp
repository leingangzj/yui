#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32Pcap — writes libpcap-format files to the SD card via M5Unified's
// SD wrapper. v0.2 Track C code.
//
// The format itself is identical between native and ESP32 (LE bytes,
// libpcap spec); FakePcap in src/hal/native/ has the canonical reference
// implementation. This class will reuse the same byte-emitting logic
// once the SD path is wired (probably by inheriting from FakePcap and
// overriding the buffer with a streaming writer — TBD).
//
// STUB: open() returns false until the SD writer lands.
#include "yui/hal/IPcap.hpp"

namespace yui {

class Esp32Pcap : public IPcap {
public:
  bool open(const char* /*path*/, PcapLinkType /*lt*/, uint32_t /*snap*/) override {
    return false;
  }
  bool write_packet(const uint8_t* /*frame*/, size_t /*len*/, uint64_t /*ts*/) override {
    return false;
  }
  void close() override {}
  uint64_t bytes_written() const override { return 0; }
};

}  // namespace yui
#endif

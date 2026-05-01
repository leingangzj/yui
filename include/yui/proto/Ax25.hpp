#pragma once
// AX.25 UI frame parser — for KISS-stripped frames coming out of a TNC
// in KISS mode. See docs/protocols/AX25_APRS.md for the byte layout.
//
// STATUS: type definitions are final; parse() is a stub that returns
// false. Implementation lands in v0.2 Track A.
#include <cstdint>
#include <cstddef>

namespace yui::ax25 {

constexpr uint8_t kControlUI = 0x03;
constexpr uint8_t kPidNoLayer3 = 0xF0;

struct Address {
  char    call[7]    = {0};   // up to 6 ASCII chars + NUL
  uint8_t ssid       = 0;     // 0..15
  bool    last       = false; // address-extension bit (byte 6, bit 0)
  bool    repeated   = false; // H-bit, set on digi addrs that retransmitted
};

struct Frame {
  Address       dst;
  Address       src;
  Address       digis[8];
  uint8_t       digi_count = 0;
  uint8_t       control    = 0;     // expect kControlUI
  uint8_t       pid        = 0;     // expect kPidNoLayer3 for APRS
  const uint8_t* info      = nullptr;
  size_t        info_len   = 0;
};

// Parse a KISS-stripped AX.25 byte buffer (no FEND, no FCS).
// Returns true on a syntactically-valid UI frame; false on malformed
// input. info points into buf, so the buf must outlive the Frame.
//
// STUB: always returns false until v0.2 Track A implementation lands.
inline bool parse(const uint8_t* /*buf*/, size_t /*len*/, Frame& /*out*/) {
  return false;
}

}  // namespace yui::ax25

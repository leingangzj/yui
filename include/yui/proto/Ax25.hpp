#pragma once
// AX.25 UI frame parser — for KISS-stripped frames coming out of a TNC
// in KISS mode. See docs/protocols/AX25_APRS.md for the byte layout.
#include <cstdint>
#include <cstddef>

namespace yui::ax25 {

constexpr uint8_t kControlUI   = 0x03;
constexpr uint8_t kPidNoLayer3 = 0xF0;
constexpr size_t  kMaxDigis    = 8;

struct Address {
  char    call[7]   = {0};   // up to 6 ASCII chars + NUL
  uint8_t ssid      = 0;     // 0..15
  bool    last      = false; // address-extension bit (byte 6, bit 0)
  bool    repeated  = false; // H-bit, set on digi addrs that retransmitted
};

struct Frame {
  Address       dst;
  Address       src;
  Address       digis[kMaxDigis];
  uint8_t       digi_count = 0;
  uint8_t       control    = 0;     // expect kControlUI
  uint8_t       pid        = 0;     // expect kPidNoLayer3 for APRS
  const uint8_t* info      = nullptr;
  size_t        info_len   = 0;
};

namespace detail {

// Decode one 7-byte AX.25 address. Returns false on malformed input.
inline bool decode_address(const uint8_t* p, Address& a) {
  size_t out = 0;
  for (int i = 0; i < 6; ++i) {
    const char ch = static_cast<char>((p[i] >> 1) & 0x7F);
    if (ch == ' ') break;       // space marks end of callsign
    // Permissive: accept any printable ASCII (callsigns, dest aliases
    // like "APRS", "APN390", etc. are all uppercase + digits).
    if (ch < 0x20 || ch > 0x7E) return false;
    a.call[out++] = ch;
  }
  a.call[out] = '\0';
  const uint8_t ssid_byte = p[6];
  a.ssid     = (ssid_byte >> 1) & 0x0F;
  a.last     = (ssid_byte & 0x01) != 0;
  a.repeated = (ssid_byte & 0x80) != 0;
  return out > 0;
}

}  // namespace detail

// Parse a KISS-stripped AX.25 byte buffer (no FEND, no FCS).
// Returns true on a syntactically-valid UI frame; false on malformed
// input. info points into buf, so the buf must outlive the Frame.
inline bool parse(const uint8_t* buf, size_t len, Frame& out) {
  // Minimum frame: dst(7) + src(7) + ctrl(1) + pid(1) = 16 bytes.
  if (!buf || len < 16) return false;

  out = {};  // reset

  // Destination — byte-6 last-bit MUST be clear (src always follows).
  if (!detail::decode_address(buf, out.dst)) return false;
  if (out.dst.last) return false;
  size_t off = 7;

  // Source.
  if (off + 7 > len) return false;
  if (!detail::decode_address(buf + off, out.src)) return false;
  off += 7;

  // Optional digipeater chain.
  if (!out.src.last) {
    while (out.digi_count < kMaxDigis) {
      if (off + 7 > len) return false;
      Address d;
      if (!detail::decode_address(buf + off, d)) return false;
      out.digis[out.digi_count++] = d;
      off += 7;
      if (d.last) break;
    }
    // If we ran out of digi slots before seeing last-bit, malformed.
    if (out.digi_count == 0 || !out.digis[out.digi_count - 1].last) {
      return false;
    }
  }

  // Control + PID + info field.
  if (off + 2 > len) return false;
  out.control  = buf[off++];
  out.pid      = buf[off++];
  out.info     = buf + off;
  out.info_len = len - off;

  return true;
}

}  // namespace yui::ax25

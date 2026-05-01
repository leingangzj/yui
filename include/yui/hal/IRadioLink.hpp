#pragma once
// IRadioLink — Bluetooth Classic SPP serial link to a Kenwood TH-D75
// (and protocol-compatible TH-D74). The radio runs in two modes on the
// same SPP socket: ASCII command/control (default), or KISS TNC after
// the host sends "TN 2,0\r". This HAL hides BT pairing/discovery and
// exposes both modes as plain byte streams.
//
// See docs/protocols/KENWOOD_THD75.md for protocol details.
#include <cstdint>
#include <cstddef>

namespace yui {

enum class RadioLinkState : uint8_t {
  Idle,         // not connected
  Connecting,   // BT pair / SPP open in progress
  CommandMode,  // SPP open, in ASCII command/control mode
  KissMode,     // SPP open, in KISS TNC binary mode
  Failed,       // last operation failed
};

class IRadioLink {
public:
  virtual ~IRadioLink() = default;

  // Discovery / connection
  virtual bool connect(const char* mac, const char* pin) = 0;
  virtual void disconnect() = 0;
  virtual RadioLinkState state() = 0;

  // ─── Command/control mode (ASCII, \r-terminated) ────────────────────
  // Send a command string (without trailing \r — the impl appends it),
  // wait for the radio's reply (also \r-terminated), copy into out_buf.
  // Returns true if a non-error reply was received within timeout_ms.
  virtual bool command(const char* cmd,
                       char* out_buf, size_t out_cap,
                       uint32_t timeout_ms) = 0;

  // ─── KISS mode ──────────────────────────────────────────────────────
  // Switch to KISS by sending "TN 2,0\r" (only valid from CommandMode).
  virtual bool enter_kiss(uint8_t vfo) = 0;
  // Switch back to command mode by sending the KISS Return frame
  // (0xC0 0xFF 0xC0). Only valid from KissMode.
  virtual bool exit_kiss() = 0;

  // Read available bytes in KISS mode. Returns bytes copied (0 = none
  // pending, -1 = link error).
  virtual int kiss_read(uint8_t* buf, size_t cap) = 0;

  // Write a fully-framed KISS frame (caller is responsible for FEND
  // wrapping and FESC escaping per AX.25 KISS spec).
  virtual bool kiss_write(const uint8_t* frame, size_t len) = 0;
};

}  // namespace yui

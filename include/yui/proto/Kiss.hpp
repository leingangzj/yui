#pragma once
// KISS framing — wraps/unwraps the FEND/FESC escape protocol that sits
// between the SPP serial link and the AX.25 frames. See
// docs/protocols/AX25_APRS.md and docs/protocols/KENWOOD_THD75.md §
// "KISS frame format".
//
// Wire format (per frame):
//   FEND  CMD  PAYLOAD…  FEND
//   0xC0  TT   (escaped)  0xC0
//
// Where CMD = (port << 4) | command. Port is 0 for our single-VFO use,
// command is one of 0x00 (Data), 0x01 (TXDelay), ..., 0xFF (Return).
//
// Escape rules INSIDE the payload:
//   0xC0  →  0xDB 0xDC   (FEND  → FESC + TFEND)
//   0xDB  →  0xDB 0xDD   (FESC  → FESC + TFESC)
#include <cstdint>
#include <cstddef>

namespace yui::kiss {

constexpr uint8_t FEND  = 0xC0;
constexpr uint8_t FESC  = 0xDB;
constexpr uint8_t TFEND = 0xDC;
constexpr uint8_t TFESC = 0xDD;

// Standard KISS commands (low nibble of CMD byte).
enum class Cmd : uint8_t {
  Data        = 0x00,
  TxDelay     = 0x01,
  Persist     = 0x02,
  SlotTime    = 0x03,
  TxTail      = 0x04,
  FullDuplex  = 0x05,
  SetHardware = 0x06,
  Return      = 0xFF,   // exits KISS mode (no payload)
};

// ─── Encoder ──────────────────────────────────────────────────────────
// Wraps a payload as a single KISS frame: FEND CMD <escaped> FEND.
// Returns bytes written, or 0 if the buffer is too small.
inline size_t encode(uint8_t cmd, const uint8_t* payload, size_t len,
                     uint8_t* out, size_t cap) {
  if (!out || cap < 3) return 0;            // FEND + CMD + FEND minimum
  size_t off = 0;
  out[off++] = FEND;
  out[off++] = cmd;
  for (size_t i = 0; i < len; ++i) {
    if (payload[i] == FEND) {
      if (off + 2 >= cap) return 0;
      out[off++] = FESC;
      out[off++] = TFEND;
    } else if (payload[i] == FESC) {
      if (off + 2 >= cap) return 0;
      out[off++] = FESC;
      out[off++] = TFESC;
    } else {
      if (off + 1 >= cap) return 0;
      out[off++] = payload[i];
    }
  }
  if (off + 1 > cap) return 0;
  out[off++] = FEND;
  return off;
}

inline size_t encode_data(const uint8_t* payload, size_t len,
                          uint8_t* out, size_t cap) {
  return encode(static_cast<uint8_t>(Cmd::Data), payload, len, out, cap);
}

// "Exit KISS" frame: FEND 0xFF FEND. Three bytes, no payload.
inline size_t encode_return(uint8_t* out, size_t cap) {
  return encode(static_cast<uint8_t>(Cmd::Return), nullptr, 0, out, cap);
}

// ─── Decoder ──────────────────────────────────────────────────────────
// Streaming decoder: feed it bytes as they arrive, it accumulates one
// frame at a time and exposes it via take_frame().
//
// State machine:
//   Idle   — waiting for opening FEND (everything before it is junk)
//   InBody — collecting payload bytes; FESC starts an escape; FEND ends
//   Esc    — next byte is the escaped form (TFEND / TFESC)
class Decoder {
public:
  static constexpr size_t kMaxFrame = 512;

  // Push one byte into the state machine. Returns true if a complete
  // frame is now available via take_frame().
  bool feed(uint8_t b) {
    switch (state_) {
      case S::Idle:
        if (b == FEND) { state_ = S::InBody; len_ = 0; }
        // anything else (junk between frames) is silently dropped
        return false;
      case S::InBody:
        if (b == FEND) {
          // Trailing FEND. Empty frame (back-to-back FENDs) just resets.
          if (len_ == 0) return false;
          // Otherwise we have a complete frame.
          have_frame_ = true;
          state_ = S::Idle;
          return true;
        }
        if (b == FESC) { state_ = S::Esc; return false; }
        if (len_ < kMaxFrame) buf_[len_++] = b;
        else                  state_ = S::Idle;   // overflow — drop
        return false;
      case S::Esc:
        if      (b == TFEND) { if (len_ < kMaxFrame) buf_[len_++] = FEND; }
        else if (b == TFESC) { if (len_ < kMaxFrame) buf_[len_++] = FESC; }
        else                 { state_ = S::Idle; return false; }  // protocol err
        state_ = S::InBody;
        return false;
    }
    return false;
  }

  // True when a frame is sitting in the buffer awaiting take_frame().
  bool ready() const { return have_frame_; }

  // Consume the most recent complete frame. Returns the CMD byte and a
  // pointer + length to the unescaped payload. Pointer is valid until
  // the next call to feed(). Returns false if no frame is ready.
  bool take_frame(uint8_t& cmd_out, const uint8_t*& payload, size_t& len) {
    if (!have_frame_ || len_ == 0) return false;
    cmd_out  = buf_[0];
    payload  = buf_ + 1;
    len      = len_ - 1;
    have_frame_ = false;
    return true;
  }

  void reset() {
    state_ = S::Idle;
    len_ = 0;
    have_frame_ = false;
  }

private:
  enum class S { Idle, InBody, Esc };
  S        state_     = S::Idle;
  size_t   len_       = 0;
  bool     have_frame_ = false;
  uint8_t  buf_[kMaxFrame];
};

}  // namespace yui::kiss

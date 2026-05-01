#pragma once
// APRS info-field decoder — parses the application-data portion of an
// AX.25 UI frame (after Ax25::parse() succeeds). v0.2 must-have scope:
// uncompressed positions only (DTI = '!' / '=' / '/' / '@') and status
// reports ('>'). Compressed (Base-91), Mic-E, weather, telemetry are
// v0.3+.
//
// See docs/protocols/AX25_APRS.md for the spec.
//
// STATUS: types final, parsers stubbed.
#include <cstdint>
#include <cstddef>

namespace yui::aprs {

enum class Dti : uint8_t {
  Unknown            = 0,
  PositionNoTime     = '!',   // 0x21
  PositionNoTimeMsg  = '=',   // 0x3D
  PositionWithTime   = '/',   // 0x2F
  PositionWithTimeMsg = '@',  // 0x40
  Status             = '>',   // 0x3E
  Object             = ';',   // 0x3B
  Message            = ':',   // 0x3A
};

inline Dti dti_of(uint8_t b) {
  switch (b) {
    case '!': return Dti::PositionNoTime;
    case '=': return Dti::PositionNoTimeMsg;
    case '/': return Dti::PositionWithTime;
    case '@': return Dti::PositionWithTimeMsg;
    case '>': return Dti::Status;
    case ';': return Dti::Object;
    case ':': return Dti::Message;
    default:  return Dti::Unknown;
  }
}

struct Position {
  double  lat_deg     = 0.0;   // signed: +N / -S
  double  lon_deg     = 0.0;   // signed: +E / -W
  char    symbol_table = 0;    // '/' (primary) or '\\' (alt) or overlay
  char    symbol_code  = 0;
  char    comment[44]  = {0};  // truncated; UTF-8 unsafe
};

// True if `info` carries an uncompressed position. Out-fields valid only
// on success. STUB returns false; impl in v0.2 Track A.
inline bool parse_position(const uint8_t* /*info*/, size_t /*info_len*/,
                           Position& /*out*/) {
  return false;
}

// True if `info` is a status report ('>' DTI). STUB returns false.
inline bool parse_status(const uint8_t* /*info*/, size_t /*info_len*/,
                         char* /*out_text*/, size_t /*cap*/) {
  return false;
}

}  // namespace yui::aprs

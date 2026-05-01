#pragma once
// APRS info-field decoder — parses the application-data portion of an
// AX.25 UI frame (after Ax25::parse() succeeds). v0.2 must-have scope:
// uncompressed positions ('!' / '=' / '/' / '@') and status reports
// ('>'). Compressed (Base-91), Mic-E, weather, telemetry are v0.3+.
//
// See docs/protocols/AX25_APRS.md for the spec.
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace yui::aprs {

enum class Dti : uint8_t {
  Unknown             = 0,
  PositionNoTime      = '!',
  PositionNoTimeMsg   = '=',
  PositionWithTime    = '/',
  PositionWithTimeMsg = '@',
  Status              = '>',
  Object              = ';',
  Message             = ':',
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
  char    comment[44]  = {0};
};

namespace detail {

inline bool digits_to_int(const uint8_t* p, int n, int& out) {
  int v = 0;
  for (int i = 0; i < n; ++i) {
    if (p[i] < '0' || p[i] > '9') return false;
    v = v * 10 + (p[i] - '0');
  }
  out = v;
  return true;
}

}  // namespace detail

// True if `info` carries an uncompressed position. Out-fields valid only
// on success. Compressed Base-91 / Mic-E formats are NOT supported here.
inline bool parse_position(const uint8_t* info, size_t info_len, Position& out) {
  if (!info || info_len < 1) return false;

  const Dti dti = dti_of(info[0]);
  size_t off = 1;

  switch (dti) {
    case Dti::PositionNoTime:
    case Dti::PositionNoTimeMsg:
      break;
    case Dti::PositionWithTime:
    case Dti::PositionWithTimeMsg:
      // Skip 7-byte timestamp ("DDHHMMz", "HHMMSSh", or "MDHM").
      if (info_len < off + 7) return false;
      off += 7;
      break;
    default:
      return false;
  }

  // Need lat(8) + sym(1) + lon(9) + sym(1) = 19 bytes.
  if (info_len < off + 19) return false;

  // Compressed format: byte at lat-position is the symbol-table char,
  // not a digit. Reject — compressed isn't in v0.2 scope.
  if (info[off] == '/' || info[off] == '\\') return false;

  // Lat: DDMM.mmN/S
  int lat_d = 0, lat_mi = 0, lat_mf = 0;
  if (!detail::digits_to_int(info + off,     2, lat_d))  return false;
  if (!detail::digits_to_int(info + off + 2, 2, lat_mi)) return false;
  if (info[off + 4] != '.')                              return false;
  if (!detail::digits_to_int(info + off + 5, 2, lat_mf)) return false;
  const char lat_hem = static_cast<char>(info[off + 7]);
  if (lat_hem != 'N' && lat_hem != 'S') return false;
  if (lat_d > 90 || lat_mi >= 60)        return false;

  out.lat_deg = static_cast<double>(lat_d) +
                (static_cast<double>(lat_mi) +
                 static_cast<double>(lat_mf) / 100.0) / 60.0;
  if (lat_hem == 'S') out.lat_deg = -out.lat_deg;

  off += 8;
  out.symbol_table = static_cast<char>(info[off++]);

  // Lon: DDDMM.mmW/E
  int lon_d = 0, lon_mi = 0, lon_mf = 0;
  if (!detail::digits_to_int(info + off,     3, lon_d))  return false;
  if (!detail::digits_to_int(info + off + 3, 2, lon_mi)) return false;
  if (info[off + 5] != '.')                              return false;
  if (!detail::digits_to_int(info + off + 6, 2, lon_mf)) return false;
  const char lon_hem = static_cast<char>(info[off + 8]);
  if (lon_hem != 'W' && lon_hem != 'E') return false;
  if (lon_d > 180 || lon_mi >= 60)       return false;

  out.lon_deg = static_cast<double>(lon_d) +
                (static_cast<double>(lon_mi) +
                 static_cast<double>(lon_mf) / 100.0) / 60.0;
  if (lon_hem == 'W') out.lon_deg = -out.lon_deg;

  off += 9;
  out.symbol_code = static_cast<char>(info[off++]);

  // Comment fills whatever's left, truncated to comment[].
  size_t cmt = (info_len > off) ? (info_len - off) : 0;
  if (cmt >= sizeof(out.comment)) cmt = sizeof(out.comment) - 1;
  std::memcpy(out.comment, info + off, cmt);
  out.comment[cmt] = '\0';

  return true;
}

// True if `info` is a status report ('>' DTI). out_text gets the
// trimmed status string (cap-1 bytes max).
inline bool parse_status(const uint8_t* info, size_t info_len,
                         char* out_text, size_t cap) {
  if (!info || info_len < 1 || !out_text || cap == 0) return false;
  if (dti_of(info[0]) != Dti::Status) return false;
  const size_t avail = info_len - 1;
  const size_t n = (avail < cap - 1) ? avail : (cap - 1);
  std::memcpy(out_text, info + 1, n);
  out_text[n] = '\0';
  return true;
}

}  // namespace yui::aprs

#pragma once
// Pcap — a thin convenience wrapper that hands packets to an IPcap
// implementation. Doesn't add logic, just a single place to centralize
// timestamps + filename conventions for v0.2 Track C.
//
// Example: writer.write_now(frame, len) tagged with the current epoch.
#include "yui/hal/IPcap.hpp"
#include <cstdio>

namespace yui::pcap {

// Filename convention for handshake captures.
//   /handshakes/<bssid>-<unix_ts>.pcap
// Sortable by time, prunable by age. ~24 chars including BSSID.
//
// Helper: format a path into out (cap chars). bssid expected as raw
// 6-byte MAC; ts is unix seconds.
inline size_t format_handshake_path(const uint8_t bssid[6], uint64_t unix_ts,
                                    char* out, size_t cap) {
  if (!out || cap == 0) return 0;
  // /handshakes/AABBCCDDEEFF-1735689600.pcap = 36 chars + NUL
  const int n = std::snprintf(
      out, cap, "/handshakes/%02X%02X%02X%02X%02X%02X-%llu.pcap",
      bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
      static_cast<unsigned long long>(unix_ts));
  return (n > 0) ? static_cast<size_t>(n) : 0;
}

}  // namespace yui::pcap

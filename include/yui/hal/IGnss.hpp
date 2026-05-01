#pragma once
// IGnss — GPS / GNSS position feed. The TH-D75 streams NMEA over its
// Bluetooth SPP link when the GP command is enabled; alternatively, a
// future onboard GNSS module could implement this same interface.
//
// Designed to be called once per UI tick — the implementation is
// expected to keep the latest fix cached internally.
#include <cstdint>

namespace yui {

struct GnssFix {
  bool   valid       = false;     // false = no fix yet, struct contents undefined
  double lat_deg     = 0.0;       // signed: +N / -S
  double lon_deg     = 0.0;       // signed: +E / -W
  float  altitude_m  = 0.0f;
  float  speed_kn    = 0.0f;      // knots
  float  course_deg  = 0.0f;      // true heading, 0..360
  uint8_t satellites = 0;
  uint64_t epoch_seconds = 0;     // UTC; 0 if not yet known
};

class IGnss {
public:
  virtual ~IGnss() = default;

  // Most recent fix (cached). Always returns a struct; check fix.valid.
  virtual GnssFix latest() = 0;

  // True if any NMEA data has been received since boot. Used by UIs to
  // distinguish "no GPS hardware" from "GPS is searching."
  virtual bool any_data_seen() const = 0;
};

}  // namespace yui

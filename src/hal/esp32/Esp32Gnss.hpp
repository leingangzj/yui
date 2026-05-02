#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32Gnss — pulls NMEA sentences from the TH-D75 over the same
// BluetoothSerial socket that Esp32RadioLink owns. Parses GGA/RMC
// for position + altitude + UTC time.
//
// Requires: Esp32RadioLink::instance() returns a connected link in
// CommandMode (i.e. radio configured with GP on, not in KISS mode).
#include "yui/hal/IGnss.hpp"
#include "yui/hal/IClock.hpp"
#include "Esp32RadioLink.hpp"
#include <cstdlib>
#include <cstring>

namespace yui {

class Esp32Gnss : public IGnss {
public:
  GnssFix latest() override {
    poll_();
    return cached_;
  }

  bool any_data_seen() const override { return any_seen_; }

private:
  void poll_() {
    auto* link = Esp32RadioLink::instance();
    if (!link) return;
    char line[120];
    while (link->peek_nmea_line(line, sizeof(line))) {
      any_seen_ = true;
      if (std::strncmp(line, "$GPRMC", 6) == 0 ||
          std::strncmp(line, "$GNRMC", 6) == 0) parse_rmc_(line);
      else if (std::strncmp(line, "$GPGGA", 6) == 0 ||
               std::strncmp(line, "$GNGGA", 6) == 0) parse_gga_(line);
    }
  }

  static bool next_field_(char*& p, char* buf, size_t cap) {
    size_t i = 0;
    while (*p && *p != ',' && *p != '*' && i + 1 < cap) buf[i++] = *p++;
    buf[i] = '\0';
    if (*p == ',') ++p;
    return i > 0 || *p != '\0';
  }

  // Parse "DDMM.mmmm" + 'N'/'S' or "DDDMM.mmmm" + 'E'/'W'
  static double parse_lat_(const char* coord, char hem) {
    if (!coord || !*coord) return 0;
    const double f = std::atof(coord);
    const int    d = static_cast<int>(f / 100);
    const double m = f - d * 100;
    double v = d + m / 60.0;
    return (hem == 'S') ? -v : v;
  }
  static double parse_lon_(const char* coord, char hem) {
    if (!coord || !*coord) return 0;
    const double f = std::atof(coord);
    const int    d = static_cast<int>(f / 100);
    const double m = f - d * 100;
    double v = d + m / 60.0;
    return (hem == 'W') ? -v : v;
  }

  // $GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E*68
  void parse_rmc_(char* line) {
    char* p = line;
    char field[32];
    next_field_(p, field, sizeof(field)); // $GPRMC
    next_field_(p, field, sizeof(field)); // time hhmmss
    next_field_(p, field, sizeof(field)); // status A/V
    if (field[0] != 'A') { cached_.valid = false; return; }
    char lat_s[16], lat_h[4], lon_s[16], lon_h[4];
    next_field_(p, lat_s, sizeof(lat_s));
    next_field_(p, lat_h, sizeof(lat_h));
    next_field_(p, lon_s, sizeof(lon_s));
    next_field_(p, lon_h, sizeof(lon_h));
    next_field_(p, field, sizeof(field)); // speed knots
    cached_.speed_kn = static_cast<float>(std::atof(field));
    next_field_(p, field, sizeof(field)); // course
    cached_.course_deg = static_cast<float>(std::atof(field));

    cached_.lat_deg = parse_lat_(lat_s, lat_h[0]);
    cached_.lon_deg = parse_lon_(lon_s, lon_h[0]);
    cached_.valid   = true;
  }

  // $GPGGA,225446,4916.45,N,12311.12,W,1,06,1.5,2.4,M,-21.0,M,,*68
  void parse_gga_(char* line) {
    char* p = line;
    char field[32];
    next_field_(p, field, sizeof(field)); // $GPGGA
    next_field_(p, field, sizeof(field)); // time
    char lat_s[16], lat_h[4], lon_s[16], lon_h[4];
    next_field_(p, lat_s, sizeof(lat_s));
    next_field_(p, lat_h, sizeof(lat_h));
    next_field_(p, lon_s, sizeof(lon_s));
    next_field_(p, lon_h, sizeof(lon_h));
    char fix_q[4]; next_field_(p, fix_q, sizeof(fix_q));
    if (fix_q[0] == '0') return;
    char sats[4]; next_field_(p, sats, sizeof(sats));
    cached_.satellites = static_cast<uint8_t>(std::atoi(sats));
    next_field_(p, field, sizeof(field));   // hdop
    next_field_(p, field, sizeof(field));   // alt
    cached_.altitude_m = static_cast<float>(std::atof(field));
    cached_.lat_deg = parse_lat_(lat_s, lat_h[0]);
    cached_.lon_deg = parse_lon_(lon_s, lon_h[0]);
    cached_.valid   = true;
  }

  GnssFix cached_;
  bool    any_seen_ = false;
};

}  // namespace yui
#endif

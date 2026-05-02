#pragma once
// Time helpers for SatTracker:
//   - Julian Date from UNIX epoch / from year+day-of-year
//   - GMST (Greenwich Mean Sidereal Time) from JD — needed to rotate
//     ECI position into ECEF for an observer's frame.
//
// Reference: Vallado, "Fundamentals of Astrodynamics and Applications,"
// 4th ed. Algorithm 14 (gstime).
#include <cmath>
#include <cstdint>

namespace yui::sat {

constexpr double kJ2000      = 2451545.0;       // JD of 2000-01-01 12:00 UTC
constexpr double kSecsPerDay = 86400.0;
constexpr double kTwoPi      = 6.283185307179586;
constexpr double kDeg2Rad    = 0.017453292519943295;
constexpr double kRad2Deg    = 57.29577951308232;

inline double jd_from_unix(double unix_sec) {
  return 2440587.5 + unix_sec / kSecsPerDay;
}

// JD from epoch year + fractional day-of-year (TLE format).
// Day 1.0 = Jan 1 00:00 UTC. Day 1.5 = Jan 1 12:00 UTC.
inline double jd_from_year_day(int year, double day_of_year) {
  // Convert year-start to JD using the standard formula.
  // JD at 0000 UTC of Jan 1 of `year`:
  const int yr = year;
  const int A = yr / 100;
  const int B = 2 - A + A / 4;
  const int day_jan1 = static_cast<int>(365.25 * (yr + 4716))
                      + static_cast<int>(30.6001 * (1 + 1)) + 1 + B - 1524;
  // The above gives JD at noon Jan 1 of year (calendar formula); correct
  // by -0.5 to get 0h UTC Jan 1.
  const double jd_year_start = day_jan1 - 0.5;
  return jd_year_start + (day_of_year - 1.0);
}

// Greenwich Mean Sidereal Time in radians (0..2π) for given JD UTC.
// Vallado eq. 3-45.
inline double gstime(double jd) {
  const double tut1 = (jd - kJ2000) / 36525.0;
  double temp = -6.2e-6 * tut1 * tut1 * tut1
              + 0.093104 * tut1 * tut1
              + (876600.0 * 3600.0 + 8640184.812866) * tut1
              + 67310.54841;
  // Convert seconds to radians
  temp = std::fmod(temp * kDeg2Rad / 240.0, kTwoPi);
  if (temp < 0.0) temp += kTwoPi;
  return temp;
}

}  // namespace yui::sat

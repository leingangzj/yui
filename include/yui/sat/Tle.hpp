#pragma once
// NORAD Two-Line Element parser. CelesTrak distributes TLEs as fixed-
// column ASCII; spec at https://celestrak.org/NORAD/documentation/tle-fmt.php
//
// Line 1 (1-indexed cols 1..69):
//   1     '1'
//   3-7   catalog (5 digits)
//   8     classification ('U'/'C'/'S')
//   10-17 international designator
//   19-20 epoch year (YY; <57 → 20YY else 19YY)
//   21-32 epoch day-of-year + fractional day
//   34-43 ndot (rev/day^2)
//   45-52 nddot — compressed (implicit decimal + signed exp)
//   54-61 BSTAR — compressed
//   63    ephemeris type
//   65-68 element-set number
//   69    checksum
//
// Line 2:
//   1     '2'
//   3-7   catalog (must match L1)
//   9-16  inclination (deg)
//   18-25 RAAN (deg)
//   27-33 eccentricity (implicit "0.")
//   35-42 arg of perigee (deg)
//   44-51 mean anomaly (deg)
//   53-63 mean motion (rev/day)
//   64-68 rev count at epoch
//   69    checksum
//
// Compressed exp like " 12345-3" = +0.12345e-3.
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace yui::sat {

struct TleElements {
  uint32_t norad_id          = 0;
  char     classification    = 'U';
  char     intl_designator[12] = {0};
  int      epoch_year        = 0;       // 4-digit
  double   epoch_day         = 0.0;     // day-of-year + fraction
  double   ndot              = 0.0;     // rev/day^2
  double   nddot             = 0.0;     // rev/day^3
  double   bstar             = 0.0;
  int      element_set       = 0;
  double   inclination_deg   = 0.0;
  double   raan_deg          = 0.0;
  double   eccentricity      = 0.0;
  double   arg_perigee_deg   = 0.0;
  double   mean_anomaly_deg  = 0.0;
  double   mean_motion       = 0.0;     // rev/day
  uint32_t rev_at_epoch      = 0;
  char     name[24]          = {0};     // optional, from line 0 if provided
};

namespace tle_detail {

inline void rstrip(char* s) {
  size_t n = std::strlen(s);
  while (n > 0 && s[n - 1] == ' ') s[--n] = '\0';
}

inline bool copy_cols(const char* src, size_t src_len,
                      int start, int end, char* dst) {
  if (start < 1 || end < start) return false;
  const size_t s = static_cast<size_t>(start - 1);
  const size_t e = static_cast<size_t>(end);
  if (e > src_len) return false;
  std::memcpy(dst, src + s, e - s);
  dst[e - s] = '\0';
  return true;
}

inline double parse_double(const char* src, size_t src_len,
                           int start, int end, bool& ok) {
  char buf[20] = {0};
  if (!copy_cols(src, src_len, start, end, buf)) { ok = false; return 0; }
  char* endp = nullptr;
  double v = std::strtod(buf, &endp);
  ok = (endp != buf);
  return v;
}

inline long parse_long(const char* src, size_t src_len,
                       int start, int end, bool& ok) {
  char buf[20] = {0};
  if (!copy_cols(src, src_len, start, end, buf)) { ok = false; return 0; }
  char* endp = nullptr;
  long v = std::strtol(buf, &endp, 10);
  ok = (endp != buf);
  return v;
}

// "  12345-3" → +0.12345e-3.   "-12345+0" → -0.12345
inline double parse_compressed(const char* field8, bool& ok) {
  ok = false;
  const char* p = field8;
  while (*p == ' ') ++p;
  int sign = 1;
  if (*p == '-') { sign = -1; ++p; }
  else if (*p == '+') ++p;
  double mant = 0;
  for (int i = 0; i < 5; ++i) {
    if (p[i] < '0' || p[i] > '9') return 0.0;
    mant = mant * 10 + (p[i] - '0');
  }
  p += 5;
  int esign = 1;
  if (*p == '-') { esign = -1; ++p; }
  else if (*p == '+') ++p;
  if (*p < '0' || *p > '9') return 0.0;
  int exp = (*p - '0');
  ok = true;
  double m = mant / 100000.0 * sign;
  double scale = 1.0;
  for (int i = 0; i < exp; ++i) scale *= 10.0;
  return (esign > 0) ? m * scale : m / scale;
}

}  // namespace tle_detail

inline bool parse_tle(const char* line1, const char* line2, TleElements& out) {
  if (!line1 || !line2) return false;
  const size_t l1 = std::strlen(line1);
  const size_t l2 = std::strlen(line2);
  if (l1 < 68 || l2 < 68) return false;
  if (line1[0] != '1' || line2[0] != '2') return false;

  bool ok = true;
  long cat1 = tle_detail::parse_long(line1, l1, 3, 7, ok); if (!ok) return false;
  long cat2 = tle_detail::parse_long(line2, l2, 3, 7, ok); if (!ok) return false;
  if (cat1 != cat2) return false;
  out.norad_id = static_cast<uint32_t>(cat1);
  out.classification = line1[7];

  char id[12] = {0};
  if (!tle_detail::copy_cols(line1, l1, 10, 17, id)) return false;
  tle_detail::rstrip(id);
  std::strncpy(out.intl_designator, id, sizeof(out.intl_designator) - 1);

  long ey = tle_detail::parse_long(line1, l1, 19, 20, ok); if (!ok) return false;
  out.epoch_year = (ey < 57) ? (2000 + static_cast<int>(ey))
                              : (1900 + static_cast<int>(ey));

  out.epoch_day = tle_detail::parse_double(line1, l1, 21, 32, ok); if (!ok) return false;
  out.ndot      = tle_detail::parse_double(line1, l1, 34, 43, ok); if (!ok) return false;

  char buf[10] = {0};
  if (!tle_detail::copy_cols(line1, l1, 45, 52, buf)) return false;
  out.nddot = tle_detail::parse_compressed(buf, ok); if (!ok) return false;
  if (!tle_detail::copy_cols(line1, l1, 54, 61, buf)) return false;
  out.bstar = tle_detail::parse_compressed(buf, ok); if (!ok) return false;

  out.element_set = static_cast<int>(
      tle_detail::parse_long(line1, l1, 65, 68, ok));

  out.inclination_deg  = tle_detail::parse_double(line2, l2,  9, 16, ok); if (!ok) return false;
  out.raan_deg         = tle_detail::parse_double(line2, l2, 18, 25, ok); if (!ok) return false;
  long ecc_raw = tle_detail::parse_long(line2, l2, 27, 33, ok); if (!ok) return false;
  out.eccentricity = static_cast<double>(ecc_raw) / 1.0e7;
  out.arg_perigee_deg  = tle_detail::parse_double(line2, l2, 35, 42, ok); if (!ok) return false;
  out.mean_anomaly_deg = tle_detail::parse_double(line2, l2, 44, 51, ok); if (!ok) return false;
  out.mean_motion      = tle_detail::parse_double(line2, l2, 53, 63, ok); if (!ok) return false;
  out.rev_at_epoch     = static_cast<uint32_t>(
      tle_detail::parse_long(line2, l2, 64, 68, ok));
  return true;
}

inline int compute_checksum(const char* line) {
  if (!line) return -1;
  int sum = 0;
  for (int i = 0; i < 68; ++i) {
    char c = line[i];
    if (c == '\0') return -1;
    if (c >= '0' && c <= '9') sum += (c - '0');
    else if (c == '-')         sum += 1;
  }
  return sum % 10;
}

inline bool checksum_ok(const char* line) {
  const int e = compute_checksum(line);
  if (e < 0) return false;
  if (std::strlen(line) < 69) return false;
  if (line[68] < '0' || line[68] > '9') return false;
  return (line[68] - '0') == e;
}

}  // namespace yui::sat

#pragma once
// Pure-logic date helpers — no system clock, no <chrono>. Everything is a
// year/month/day triple plus a day-of-week derivation via Zeller's
// congruence. Used by CalendarApp; native-testable.
#include <cstdint>

namespace yui::date {

constexpr bool is_leap(int y) {
  return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

constexpr int days_in_month(int y, int m) {
  constexpr int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (m < 1 || m > 12) return 0;
  if (m == 2 && is_leap(y)) return 29;
  return kDays[m - 1];
}

// Zeller's congruence — returns 0=Sunday..6=Saturday.
constexpr int day_of_week(int y, int m, int d) {
  if (m < 3) { m += 12; y -= 1; }
  const int K = y % 100;
  const int J = y / 100;
  const int h = (d + 13 * (m + 1) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
  // Zeller's h: 0=Saturday, 1=Sunday..6=Friday. Remap to 0=Sunday..6=Saturday.
  return (h + 6) % 7;
}

struct Date { int y; int m; int d; };

inline Date add_days(Date date, int delta) {
  int d = date.d + delta;
  int m = date.m;
  int y = date.y;
  while (d < 1) {
    --m;
    if (m < 1) { m = 12; --y; }
    d += days_in_month(y, m);
  }
  while (d > days_in_month(y, m)) {
    d -= days_in_month(y, m);
    ++m;
    if (m > 12) { m = 1; ++y; }
  }
  return {y, m, d};
}

inline Date add_months(Date date, int delta) {
  int total = date.y * 12 + (date.m - 1) + delta;
  int y = total / 12;
  int m = (total % 12) + 1;
  if (m < 1) { m += 12; --y; }
  int d = date.d;
  const int max_d = days_in_month(y, m);
  if (d > max_d) d = max_d;
  return {y, m, d};
}

}  // namespace yui::date

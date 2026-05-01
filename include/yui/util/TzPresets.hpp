#pragma once
// Curated POSIX TZ presets. Format follows IEEE 1003.1: name + offset, with
// optional DST rules. Fed to configTzTime on ESP32 / setenv("TZ",...) +
// tzset on native.
#include <cstddef>
#include <cstring>

namespace yui {

struct TzPreset {
  const char* label;   // human label rendered in Settings
  const char* posix;   // value passed to configTzTime / setenv
};

inline const TzPreset* tz_presets(size_t& count) {
  static const TzPreset kPresets[] = {
    {"UTC",         "UTC0"},
    {"US Eastern",  "EST5EDT,M3.2.0,M11.1.0"},
    {"US Central",  "CST6CDT,M3.2.0,M11.1.0"},
    {"US Mountain", "MST7MDT,M3.2.0,M11.1.0"},
    {"US Pacific",  "PST8PDT,M3.2.0,M11.1.0"},
    {"UK / Lisbon", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Cent Europe", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Japan",       "JST-9"},
    {"Sydney",      "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  };
  count = sizeof(kPresets) / sizeof(kPresets[0]);
  return kPresets;
}

// Find the index of a preset whose posix matches `posix` exactly.
// Returns 0 (UTC) on miss so the picker has something to highlight.
inline size_t tz_index_of(const char* posix) {
  if (!posix) return 0;
  size_t n = 0;
  const TzPreset* p = tz_presets(n);
  for (size_t i = 0; i < n; ++i) {
    if (std::strcmp(p[i].posix, posix) == 0) return i;
  }
  return 0;
}

}  // namespace yui

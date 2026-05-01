#pragma once
// Curated NTP server presets — fed to configTzTime on ESP32 / surfaced in
// SettingsApp.
#include <cstddef>
#include <cstring>

namespace yui {

struct NtpPreset {
  const char* label;
  const char* host;
};

inline const NtpPreset* ntp_presets(size_t& count) {
  static const NtpPreset kPresets[] = {
    {"pool.ntp",   "pool.ntp.org"},
    {"Cloudflare", "time.cloudflare.com"},
    {"Google",     "time.google.com"},
    {"NIST",       "time.nist.gov"},
  };
  count = sizeof(kPresets) / sizeof(kPresets[0]);
  return kPresets;
}

inline size_t ntp_index_of(const char* host) {
  if (!host) return 0;
  size_t n = 0;
  const NtpPreset* p = ntp_presets(n);
  for (size_t i = 0; i < n; ++i) {
    if (std::strcmp(p[i].host, host) == 0) return i;
  }
  return 0;
}

}  // namespace yui

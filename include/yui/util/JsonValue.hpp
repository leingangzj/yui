#pragma once
// Tiny JSON value extractor — purpose-built for the small flat objects
// the Pineapple REST API returns. Not a full parser. Looks for the key
// in the source as a literal `"<key>":<value>` and grabs the value.
//
// Limitations (intentional):
//   - assumes input is well-formed UTF-8 JSON
//   - does NOT handle JSON escape sequences in keys or string values
//     (e.g. \" inside a value); we don't see those in Pineapple output
//   - does NOT recurse into nested objects (caller substr's first)
//
// Sufficient for our shape budget. If we ever need general JSON, swap
// to ArduinoJson or cJSON.
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace yui::json {

namespace detail {

// Find a substring; returns pointer or nullptr. Treats src as a C
// string (NUL-terminated).
inline const char* find_token(const char* src, const char* tok) {
  return src ? std::strstr(src, tok) : nullptr;
}

// Skip ASCII whitespace.
inline const char* skip_ws(const char* p) {
  if (!p) return nullptr;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
  return p;
}

}  // namespace detail

// Find `"<key>":` and copy the next string value into `out` (up to cap-1
// bytes). Returns true on success.
inline bool find_string(const char* src, const char* key,
                        char* out, size_t cap) {
  if (!src || !key || !out || cap == 0) return false;
  // Build the search pattern "key": (with quotes around key)
  char pat[64];
  const int n = std::snprintf(pat, sizeof(pat), "\"%s\"", key);
  if (n <= 0 || n >= static_cast<int>(sizeof(pat))) return false;
  const char* hit = detail::find_token(src, pat);
  if (!hit) return false;
  hit += n;
  hit = detail::skip_ws(hit);
  if (!hit || *hit != ':') return false;
  ++hit;
  hit = detail::skip_ws(hit);
  if (!hit || *hit != '"') return false;
  ++hit;
  size_t i = 0;
  while (*hit && *hit != '"' && i + 1 < cap) {
    out[i++] = *hit++;
  }
  out[i] = '\0';
  return *hit == '"';
}

// Find `"<key>":<integer>`. Returns true on success; sets *out_val.
inline bool find_int(const char* src, const char* key, int* out_val) {
  if (!src || !key || !out_val) return false;
  char pat[64];
  const int n = std::snprintf(pat, sizeof(pat), "\"%s\"", key);
  if (n <= 0 || n >= static_cast<int>(sizeof(pat))) return false;
  const char* hit = detail::find_token(src, pat);
  if (!hit) return false;
  hit += n;
  hit = detail::skip_ws(hit);
  if (!hit || *hit != ':') return false;
  ++hit;
  hit = detail::skip_ws(hit);
  if (!hit) return false;
  // Allow numeric string-form like "42" too.
  bool quoted = (*hit == '"');
  if (quoted) ++hit;
  bool neg = false;
  if (*hit == '-') { neg = true; ++hit; }
  if (*hit < '0' || *hit > '9') return false;
  int v = 0;
  while (*hit >= '0' && *hit <= '9') { v = v * 10 + (*hit - '0'); ++hit; }
  *out_val = neg ? -v : v;
  return true;
}

// Find `"<key>":<bool>`. Returns true on success.
inline bool find_bool(const char* src, const char* key, bool* out_val) {
  if (!src || !key || !out_val) return false;
  char pat[64];
  const int n = std::snprintf(pat, sizeof(pat), "\"%s\"", key);
  if (n <= 0 || n >= static_cast<int>(sizeof(pat))) return false;
  const char* hit = detail::find_token(src, pat);
  if (!hit) return false;
  hit += n;
  hit = detail::skip_ws(hit);
  if (!hit || *hit != ':') return false;
  ++hit;
  hit = detail::skip_ws(hit);
  if (!hit) return false;
  if (std::strncmp(hit, "true", 4) == 0)  { *out_val = true;  return true; }
  if (std::strncmp(hit, "false", 5) == 0) { *out_val = false; return true; }
  return false;
}

}  // namespace yui::json

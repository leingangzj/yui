#pragma once
// SubFile — Flipper Zero `.sub` file reader / writer.
//
// Flipper's sub-GHz capture format is a plain-text key/value file. The
// most useful subset for our needs:
//
//   Filetype: Flipper SubGhz RAW File
//   Version: 1
//   Frequency: 433920000
//   Preset: FuriHalSubGhzPresetOok650Async
//   Protocol: RAW
//   RAW_Data: 320 -130 196 -118 ...   (one or more lines of int32 µs deltas;
//                                      positive = TX-on, negative = TX-off)
//
// Sharing this format means signals captured on Yui drop straight into
// the Flipper community library and vice-versa. We expose just enough to
// round-trip a capture through SD and back into TX.
//
// All parsing is allocation-friendly: writer takes a span, reader takes
// a callback for each timing delta so we don't have to pre-allocate.

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace yui::proto {

enum class SubPreset : uint8_t {
  Ook650Async,    // garage doors, fixed-code remotes
  Ook270Async,    // narrower-bandwidth OOK
  FskDev238Async, // weather sensors, narrow FSK
  FskDev476Async, // some IoT
  Custom,         // chip-specific settings provided externally
};

inline const char* sub_preset_str(SubPreset p) {
  switch (p) {
    case SubPreset::Ook650Async:    return "FuriHalSubGhzPresetOok650Async";
    case SubPreset::Ook270Async:    return "FuriHalSubGhzPresetOok270Async";
    case SubPreset::FskDev238Async: return "FuriHalSubGhzPresetFskDev238Async";
    case SubPreset::FskDev476Async: return "FuriHalSubGhzPresetFskDev476Async";
    case SubPreset::Custom:         return "FuriHalSubGhzPresetCustom";
  }
  return "FuriHalSubGhzPresetOok650Async";
}

inline SubPreset sub_preset_from(const char* s) {
  if (!s) return SubPreset::Ook650Async;
  if (std::strstr(s, "Ook650"))    return SubPreset::Ook650Async;
  if (std::strstr(s, "Ook270"))    return SubPreset::Ook270Async;
  if (std::strstr(s, "Dev238"))    return SubPreset::FskDev238Async;
  if (std::strstr(s, "Dev476"))    return SubPreset::FskDev476Async;
  if (std::strstr(s, "Custom"))    return SubPreset::Custom;
  return SubPreset::Ook650Async;
}

struct SubHeader {
  uint32_t   frequency_hz = 433920000;
  SubPreset  preset       = SubPreset::Ook650Async;
  std::string protocol    = "RAW";
};

// Build a complete .sub file from a header + a sequence of int32 µs
// timing deltas (Flipper RAW format: alternating on/off durations,
// positive=on, negative=off).
inline std::string write_sub(const SubHeader& h,
                             const int32_t* timings, std::size_t n,
                             std::size_t per_line = 64) {
  std::string out;
  out.reserve(256 + n * 8);
  out += "Filetype: Flipper SubGhz RAW File\n";
  out += "Version: 1\n";
  char buf[64];
  std::snprintf(buf, sizeof(buf), "Frequency: %u\n",
                static_cast<unsigned>(h.frequency_hz));
  out += buf;
  out += "Preset: ";
  out += sub_preset_str(h.preset);
  out += "\nProtocol: ";
  out += h.protocol;
  out += "\nRAW_Data:";
  for (std::size_t i = 0; i < n; ++i) {
    if (i % per_line == 0 && i > 0) {
      out += "\nRAW_Data:";
    }
    std::snprintf(buf, sizeof(buf), " %d", static_cast<int>(timings[i]));
    out += buf;
  }
  out += "\n";
  return out;
}

// Parse a .sub file. Calls `on_timing` once per int32 timing value.
// Returns true if the header was recognised. Tolerates missing fields
// (defaults are used) and extra unknown lines (Flipper firmware adds
// vendor-specific keys we ignore).
template <typename TimingFn>
bool read_sub(const char* text, std::size_t len,
              SubHeader& out_header, TimingFn on_timing) {
  if (!text || len == 0) return false;
  bool sawFiletype = false;
  std::size_t i = 0;
  auto eat_line = [&](std::string& line) {
    line.clear();
    while (i < len && text[i] != '\n') {
      if (text[i] != '\r') line += text[i];
      ++i;
    }
    if (i < len) ++i;  // skip \n
  };
  std::string line;
  while (i < len) {
    eat_line(line);
    if (line.empty()) continue;

    auto starts_with = [&](const char* p) {
      const std::size_t pn = std::strlen(p);
      return line.size() >= pn && std::memcmp(line.data(), p, pn) == 0;
    };

    if (starts_with("Filetype:")) {
      sawFiletype = true;
      continue;
    }
    if (starts_with("Frequency:")) {
      out_header.frequency_hz = static_cast<uint32_t>(
          std::strtoul(line.c_str() + 10, nullptr, 10));
      continue;
    }
    if (starts_with("Preset:")) {
      out_header.preset = sub_preset_from(line.c_str() + 7);
      continue;
    }
    if (starts_with("Protocol:")) {
      // Trim leading spaces
      const char* p = line.c_str() + 9;
      while (*p == ' ' || *p == '\t') ++p;
      out_header.protocol = p;
      continue;
    }
    if (starts_with("RAW_Data:")) {
      const char* p = line.c_str() + 9;
      while (*p) {
        while (*p == ' ' || *p == '\t') ++p;
        if (!*p) break;
        char* end = nullptr;
        const long v = std::strtol(p, &end, 10);
        if (end == p) break;  // no digits
        on_timing(static_cast<int32_t>(v));
        p = end;
      }
    }
  }
  return sawFiletype;
}

// Convenience: read into a vector<int32_t>.
inline bool read_sub_to_vector(const char* text, std::size_t len,
                               SubHeader& out_header,
                               std::vector<int32_t>& out_timings) {
  out_timings.clear();
  return read_sub(text, len, out_header,
                  [&](int32_t v) { out_timings.push_back(v); });
}

}  // namespace yui::proto

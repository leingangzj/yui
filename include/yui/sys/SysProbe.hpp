#pragma once
// SysProbe — runtime callbacks the system provides to apps that need to
// query the device (battery, heap, IP, RSSI, uptime). Defined here rather
// than in SysinfoApp so multiple apps and the launcher can share one type.
#include <cstdint>
#include <functional>

namespace yui {

struct SysProbe {
  std::function<int()>           battery_pct;       // 0-100, -1 if unknown
  std::function<uint32_t()>      free_heap_bytes;
  std::function<uint32_t()>      uptime_ms;
  std::function<const char*()>   ip_or_empty;
  std::function<int()>           wifi_rssi;         // dBm, 0 if disconnected
  std::function<uint64_t()>      epoch_seconds;     // 0 if not NTP-synced
};

}  // namespace yui

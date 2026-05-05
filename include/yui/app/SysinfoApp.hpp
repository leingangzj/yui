#pragma once
// Read-only system info — battery %, free heap, uptime, IP if connected.
// All readings flow through small probe lambdas so the app is testable
// without M5/Arduino headers in the native env.
#include "yui/app/App.hpp"
#include "yui/sys/SysProbe.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <ctime>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class SysinfoApp : public App {
public:
  explicit SysinfoApp(SysProbe probe) : probe_(std::move(probe)) {}
  const char* name() const override { return "Sysinfo"; }
  Category    category() const override { return Category::System; }
  const assets::IconRef* icon() const override { return &assets::icons::kInfo(); }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, "Sysinfo");

    int y = 22;
    char line[40];

    const int batt = probe_.battery_pct ? probe_.battery_pct() : -1;
    if (batt >= 0) std::snprintf(line, sizeof(line), "Battery   %3d%%", batt);
    else           std::snprintf(line, sizeof(line), "Battery   --");
    d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface); y += 14;

    if (probe_.free_heap_bytes) {
      const uint32_t fh = probe_.free_heap_bytes();
      std::snprintf(line, sizeof(line), "Free heap %u KB", static_cast<unsigned>(fh / 1024));
      d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface);
    }
    y += 14;

    if (probe_.uptime_ms) {
      const uint32_t s = probe_.uptime_ms() / 1000;
      const uint32_t hh = s / 3600, mm = (s / 60) % 60, ss = s % 60;
      std::snprintf(line, sizeof(line), "Uptime    %02u:%02u:%02u",
                    static_cast<unsigned>(hh),
                    static_cast<unsigned>(mm),
                    static_cast<unsigned>(ss));
      d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface);
    }
    y += 14;

    if (probe_.ip_or_empty) {
      const char* ip = probe_.ip_or_empty();
      std::snprintf(line, sizeof(line), "IP        %s", (ip && *ip) ? ip : "—");
      d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface);
    }
    y += 14;

    if (probe_.wifi_rssi) {
      const int r = probe_.wifi_rssi();
      if (r) std::snprintf(line, sizeof(line), "WiFi RSSI %d", r);
      else   std::snprintf(line, sizeof(line), "WiFi      off");
      d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface);
    }
    y += 14;

    if (probe_.epoch_seconds) {
      const uint64_t e = probe_.epoch_seconds();
      if (e == 0) {
        std::snprintf(line, sizeof(line), "Time      no NTP sync");
      } else {
        const std::time_t t = static_cast<std::time_t>(e);
        std::tm tm_buf{};
#if defined(_WIN32)
        localtime_s(&tm_buf, &t);
#else
        localtime_r(&t, &tm_buf);
#endif
        std::snprintf(line, sizeof(line), "Time      %02d:%02d:%02d",
                      tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
      }
      d.draw_text(8, y, line, ui::kOnSurface, ui::kSurface);
    }

    d.flush();
  }

private:
  SysProbe probe_;
};

}  // namespace yui

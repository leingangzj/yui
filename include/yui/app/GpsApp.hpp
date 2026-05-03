#pragma once
// GpsApp — show the current GNSS fix and (optionally) capture
// trackpoints to RAM, dumpable to a GPX file on Tab.
//
// Reads from IGnss; on the ESP32, that's eventually fed by the TH-D75's
// onboard GPS over the same SPP link. v0.2 stub returns an invalid fix;
// once Esp32Gnss lands, this app starts showing real positions.
//
// Keys:
//   Tab   : toggle recording; on stop, flush track to /yui-tracks/<ts>.gpx
//   Esc   : back
//
// Track buffer is intentionally bounded (kMaxPoints) — long sessions
// will need v0.3's append-mode IFs.
#include "yui/app/App.hpp"
#include "yui/hal/IGnss.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class GpsApp : public App {
public:
  static constexpr size_t kMaxPoints = 64;
  static constexpr uint32_t kRecordIntervalMs = 5000;
  static constexpr size_t kGpxBufSize = 4096;   // ~64B/pt × 64 pts

  struct Point {
    double   lat_deg;
    double   lon_deg;
    float    altitude_m;
    uint64_t epoch_seconds;
  };

  GpsApp(IGnss& gnss, IFs& fs) : gnss_(gnss), fs_(fs) {}

  const char* name() const override { return "GPS"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& hal) override {
    hal_       = &hal;
    recording_ = false;
    point_count_ = 0;
    last_record_ms_ = 0;
    last_save_ok_ = false;
  }

  void tick(uint32_t now_ms) override {
    if (!recording_) return;
    if (now_ms - last_record_ms_ < kRecordIntervalMs && last_record_ms_ != 0) return;
    last_record_ms_ = now_ms == 0 ? 1 : now_ms;
    GnssFix f = gnss_.latest();
    if (!f.valid) return;
    if (point_count_ < kMaxPoints) {
      Point& p = points_[point_count_++];
      p.lat_deg = f.lat_deg;
      p.lon_deg = f.lon_deg;
      p.altitude_m = f.altitude_m;
      p.epoch_seconds = f.epoch_seconds;
    }
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Tab) {
      if (recording_) {
        recording_ = false;
        flush_to_gpx_();
      } else {
        recording_ = true;
        point_count_ = 0;
        last_record_ms_ = 0;
        last_save_ok_ = false;
      }
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, recording_ ? "GPS  REC" : "GPS");

    GnssFix f = gnss_.latest();
    char line[40];
    int y = 22;
    if (!f.valid) {
      d.draw_text(8, y, gnss_.any_data_seen() ? "Searching..."
                                              : "No GPS data",
                  kJapanRedDark, kWhite);
      y += 14;
    } else {
      std::snprintf(line, sizeof(line), "Lat: %+10.5f", f.lat_deg);
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
      std::snprintf(line, sizeof(line), "Lon: %+10.5f", f.lon_deg);
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
      std::snprintf(line, sizeof(line), "Alt: %7.1f m  Sats: %u",
                    static_cast<double>(f.altitude_m),
                    static_cast<unsigned>(f.satellites));
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
      std::snprintf(line, sizeof(line), "Speed: %5.1f kn  Course: %5.1f",
                    static_cast<double>(f.speed_kn),
                    static_cast<double>(f.course_deg));
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
    }

    std::snprintf(line, sizeof(line), "Track: %u/%u %s",
                  static_cast<unsigned>(point_count_),
                  static_cast<unsigned>(kMaxPoints),
                  last_save_ok_ ? "saved" : "");
    d.draw_text(8, y, line, kJapanRedDark, kWhite);

    d.draw_text(8, d.height() - 14,
                recording_ ? "Tab:stop+save  Esc:back"
                           : "Tab:rec        Esc:back",
                kJapanRedDark, kWhite);
    d.flush();
  }

  // Test hooks
  bool   recording()    const { return recording_; }
  size_t point_count()  const { return point_count_; }
  bool   last_save_ok() const { return last_save_ok_; }

private:
  bool flush_to_gpx_() {
    if (point_count_ == 0) return false;
    fs_.init();
    char path[64];
    const uint64_t ts = points_[0].epoch_seconds;
    std::snprintf(path, sizeof(path), "/yui-tracks/%llu.gpx",
                  static_cast<unsigned long long>(ts));
    char buf[kGpxBufSize];
    int off = 0;
    off += std::snprintf(buf + off, sizeof(buf) - off,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<gpx version=\"1.1\" creator=\"Yui\">\n  <trk><trkseg>\n");
    for (size_t i = 0; i < point_count_ && off < static_cast<int>(sizeof(buf)) - 256; ++i) {
      const Point& p = points_[i];
      off += std::snprintf(buf + off, sizeof(buf) - off,
          "    <trkpt lat=\"%.6f\" lon=\"%.6f\"><ele>%.1f</ele></trkpt>\n",
          p.lat_deg, p.lon_deg, static_cast<double>(p.altitude_m));
    }
    off += std::snprintf(buf + off, sizeof(buf) - off,
        "  </trkseg></trk>\n</gpx>\n");
    last_save_ok_ = fs_.write_all(path, buf, static_cast<size_t>(off));
    return last_save_ok_;
  }

  IGnss&  gnss_;
  IFs&    fs_;
  Hal*    hal_           = nullptr;
  bool    recording_     = false;
  size_t  point_count_   = 0;
  uint32_t last_record_ms_ = 0;
  bool    last_save_ok_  = false;
  Point   points_[kMaxPoints];
};

}  // namespace yui

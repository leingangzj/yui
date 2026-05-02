#pragma once
// SatTrackerApp — favorites list + next-pass predictor + live-pass
// view with Doppler-corrected CAT control of the TH-D75. v0.3 headline.
//
// Three views:
//   List    — scroll favorite sats; each row shows next-pass countdown
//   Detail  — selected sat's next pass: AOS/MAX/LOS time + max el
//   Live    — pass-in-progress: az/el ticker, Doppler-corrected freq
//             written to the radio every kDopplerIntervalMs
//
// TLE source order (first hit wins):
//   1. /yui-tles.txt on SD card (3-line records: name, l1, l2)
//   2. Built-in hardcoded defaults (ISS, SO-50, AO-91 — current as of
//      docs — user is expected to override via SD with fresher TLEs)
#include "yui/app/App.hpp"
#include "yui/hal/IRadioLink.hpp"
#include "yui/hal/IGnss.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/hal/IClock.hpp"
#include "yui/sat/Tle.hpp"
#include "yui/sat/Propagator.hpp"
#include "yui/sat/Topo.hpp"
#include "yui/sat/Pass.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class SatTrackerApp : public App {
public:
  static constexpr size_t   kMaxSats          = 8;
  static constexpr uint32_t kDopplerIntervalMs = 2000;
  static constexpr const char* kTlePath        = "/yui-tles.txt";

  enum class View : uint8_t { List, Detail, Live };

  struct Favorite {
    char             name[24]   = {0};
    sat::TleElements tle;
    sat::Propagator  prop;
    sat::PassInfo    cached_pass;
    double           freq_hz    = 0.0;     // RX center freq for Doppler
    bool             valid      = false;
  };

  SatTrackerApp(IRadioLink& radio, IGnss& gnss, IFs& fs, IClock& clock)
      : radio_(radio), gnss_(gnss), fs_(fs), clock_(clock) {}

  const char* name() const override { return "SatTracker"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& hal) override {
    hal_     = &hal;
    view_    = View::List;
    cursor_  = 0;
    last_doppler_ms_ = 0;
    if (!loaded_) {
      load_tles_();
      loaded_ = true;
    }
    // Refresh per-sat next-pass info now that we're at this view.
    refresh_observer_();
    for (size_t i = 0; i < count_; ++i) refresh_pass_(i);
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (view_ == View::List) {
      if (k.key == Key::Up   && cursor_ > 0)             --cursor_;
      if (k.key == Key::Down && cursor_ + 1 < count_)    ++cursor_;
      if (k.key == Key::Enter) view_ = View::Detail;
    } else if (view_ == View::Detail) {
      if (k.key == Key::Backspace || k.key == Key::Esc) view_ = View::List;
      if (k.key == Key::Tab) view_ = View::Live;
    } else if (view_ == View::Live) {
      if (k.key == Key::Backspace || k.key == Key::Esc) view_ = View::Detail;
    }
  }

  void tick(uint32_t now_ms) override {
    if (view_ != View::Live) return;
    if (cursor_ >= count_) return;
    if (now_ms - last_doppler_ms_ < kDopplerIntervalMs && last_doppler_ms_ != 0)
      return;
    last_doppler_ms_ = now_ms == 0 ? 1 : now_ms;
    push_doppler_();
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "SatTracker", kWhite, kJapanRed);

    if (count_ == 0) {
      d.draw_text(8, 36, "No TLEs loaded", kJapanRedBright, kWhite);
      d.draw_text(8, 56, "Put 3-line records in", kJapanRedDark, kWhite);
      d.draw_text(8, 70, kTlePath, kJapanRedDark, kWhite);
      d.flush();
      return;
    }

    switch (view_) {
      case View::List:   render_list_(d);   break;
      case View::Detail: render_detail_(d); break;
      case View::Live:   render_live_(d);   break;
    }
    d.flush();
  }

  // Test hooks
  size_t   favorite_count()   const { return count_; }
  View     view()             const { return view_; }
  const Favorite& favorite_at(size_t i) const { return favs_[i]; }
  size_t   cursor()           const { return cursor_; }
  size_t   doppler_writes()   const { return doppler_writes_; }
  void     reset_loaded()           { loaded_ = false; }

private:
  // ─── TLE loading ──────────────────────────────────────────────────
  void load_tles_() {
    count_ = 0;
    if (load_tles_from_sd_()) return;
    load_tles_defaults_();
  }

  bool load_tles_from_sd_() {
    if (!fs_.exists(kTlePath)) return false;
    char buf[2048];
    const int n = fs_.read_all(kTlePath, buf, sizeof(buf));
    if (n <= 0) return false;
    return parse_tle_lines_(buf);
  }

  void load_tles_defaults_() {
    // Hardcoded sample — update when stale. Real users override via SD.
    static const char* k[] = {
      "ISS (ZARYA)",
      "1 25544U 98067A   24001.50000000  .00012345  00000-0  22345-3 0  9999",
      "2 25544  51.6400 123.4567 0001234 234.5678 125.4321 15.50000000123456",
      "SO-50",
      "1 27607U 02058C   24001.50000000  .00000123  00000-0  12345-4 0  9999",
      "2 27607  64.5500 100.0000 0050000  90.0000 270.0000 14.78000000111111",
    };
    static const double kFreqs[] = {
      145825000.0,    // ISS APRS digipeater
      436795000.0,    // SO-50 downlink
    };
    char block[256];
    for (size_t i = 0; i + 2 < sizeof(k) / sizeof(k[0]); i += 3) {
      std::snprintf(block, sizeof(block), "%s\n%s\n%s\n", k[i], k[i+1], k[i+2]);
      add_tle_record_(block, kFreqs[i / 3]);
    }
  }

  bool parse_tle_lines_(const char* buf) {
    const char* p = buf;
    while (*p && count_ < kMaxSats) {
      // Skip blank lines
      while (*p == '\n' || *p == '\r' || *p == ' ') ++p;
      if (!*p) break;
      // Take next 3 lines
      char block[260] = {0};
      int written = 0;
      int line_count = 0;
      while (*p && line_count < 3 && written < static_cast<int>(sizeof(block)) - 1) {
        block[written++] = *p;
        if (*p == '\n') ++line_count;
        ++p;
      }
      block[written] = '\0';
      if (line_count >= 2) add_tle_record_(block, 145825000.0);  // default freq
    }
    return count_ > 0;
  }

  void add_tle_record_(const char* block, double freq_hz) {
    if (count_ >= kMaxSats) return;
    char name[24] = {0};
    char l1[80] = {0};
    char l2[80] = {0};
    // Pull three newline-delimited lines.
    const char* p = block;
    auto take_line = [&p](char* dst, size_t cap) {
      size_t i = 0;
      while (*p && *p != '\n' && i + 1 < cap) dst[i++] = *p++;
      if (*p == '\n') ++p;
      while (i > 0 && (dst[i-1] == '\r' || dst[i-1] == ' ')) dst[--i] = '\0';
      dst[i] = '\0';
    };
    take_line(name, sizeof(name));
    take_line(l1,   sizeof(l1));
    take_line(l2,   sizeof(l2));
    // If first line looks like a TLE line ("1 ..."), shift: no name.
    if (l1[0] == '\0' && name[0] == '1') {
      std::strncpy(l2, l1, sizeof(l2));   // shouldn't trigger; defensive
    }
    Favorite& f = favs_[count_];
    std::strncpy(f.name, name[0] ? name : "(unnamed)", sizeof(f.name) - 1);
    f.freq_hz = freq_hz;
    if (sat::parse_tle(l1, l2, f.tle) && f.prop.init(f.tle)) {
      f.valid = true;
      ++count_;
    } else {
      f = Favorite{};   // discard
    }
  }

  // ─── Pass refresh ────────────────────────────────────────────────
  void refresh_observer_() {
    GnssFix fx = gnss_.latest();
    if (fx.valid) {
      observer_.lat_deg = fx.lat_deg;
      observer_.lon_deg = fx.lon_deg;
      observer_.alt_m   = fx.altitude_m;
    }
    // Build a JD for "now" — use clock_.epoch_seconds() if synced,
    // else fall back to TLE epoch (predictions become "next pass after
    // TLE epoch" which is still valid demo data).
    const uint64_t now = clock_.epoch_seconds();
    if (now > 0) {
      now_jd_ = sat::jd_from_unix(static_cast<double>(now));
    } else if (count_ > 0) {
      now_jd_ = sat::jd_from_year_day(favs_[0].tle.epoch_year,
                                       favs_[0].tle.epoch_day);
    } else {
      now_jd_ = sat::kJ2000;
    }
  }

  void refresh_pass_(size_t i) {
    if (i >= count_ || !favs_[i].valid) return;
    favs_[i].cached_pass = sat::predict_next_pass(
        favs_[i].prop, observer_, now_jd_, 24.0, 0.0, 30.0);
  }

  // ─── Live pass + Doppler ─────────────────────────────────────────
  void push_doppler_() {
    if (radio_.state() != RadioLinkState::CommandMode) return;
    const Favorite& f = favs_[cursor_];
    if (!f.valid || f.freq_hz <= 0) return;
    refresh_observer_();
    sat::StateVector sv;
    const double t_sec = (now_jd_ -
        sat::jd_from_year_day(f.tle.epoch_year, f.tle.epoch_day)) * sat::kSecsPerDay;
    if (!f.prop.propagate(t_sec, sv)) return;
    const sat::LookAngles la = sat::look_angles(sv, observer_, now_jd_);
    const double df = sat::doppler_hz(f.freq_hz, la.range_rate_kms);
    const long long shifted = static_cast<long long>(f.freq_hz + df);
    char cmd[32];
    std::snprintf(cmd, sizeof(cmd), "FQ 0,%010lld", shifted);
    char resp[40] = {0};
    if (radio_.command(cmd, resp, sizeof(resp), 1000)) ++doppler_writes_;
  }

  // ─── Renders ─────────────────────────────────────────────────────
  void render_list_(IDisplay& d) {
    char line[40];
    for (size_t i = 0; i < count_ && i < 6; ++i) {
      const int y = 22 + static_cast<int>(i) * 16;
      const bool sel = (i == cursor_);
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : kBlack;
      if (sel) d.fill_rect({0, y - 2, d.width(), 16}, bg);
      const Favorite& f = favs_[i];
      double mins_to = (f.cached_pass.found)
                       ? (f.cached_pass.aos_jd - now_jd_) * 24.0 * 60.0
                       : -1.0;
      if (mins_to >= 0) std::snprintf(line, sizeof(line), "%-12s %4.0fm",
                                      f.name, mins_to);
      else              std::snprintf(line, sizeof(line), "%-12s   --", f.name);
      d.draw_text(4, y + 3, line, fg, bg);
    }
    d.draw_text(8, d.height() - 14,
                "Enter:detail  Esc:back",
                kJapanRedDark, kWhite);
  }

  void render_detail_(IDisplay& d) {
    if (cursor_ >= count_) return;
    const Favorite& f = favs_[cursor_];
    char line[40];
    int y = 22;
    std::snprintf(line, sizeof(line), "%s", f.name);
    d.draw_text(8, y, line, kJapanRed, kWhite); y += 14;
    if (f.cached_pass.found) {
      const double mins_to_aos = (f.cached_pass.aos_jd - now_jd_) * 1440.0;
      const double dur_sec     = (f.cached_pass.los_jd - f.cached_pass.aos_jd)
                                  * sat::kSecsPerDay;
      std::snprintf(line, sizeof(line), "AOS in %.1f min", mins_to_aos);
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
      std::snprintf(line, sizeof(line), "Az: %.0f deg", f.cached_pass.aos_azimuth_deg);
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
      std::snprintf(line, sizeof(line), "MaxEl: %.0f deg", f.cached_pass.max_elevation_deg);
      d.draw_text(8, y, line, kBlack, kWhite); y += 14;
      std::snprintf(line, sizeof(line), "Pass: %.0f sec", dur_sec);
      d.draw_text(8, y, line, kBlack, kWhite);
    } else {
      d.draw_text(8, y, "No pass in 24h", kJapanRedDark, kWhite);
    }
    d.draw_text(8, d.height() - 14, "Tab:live Esc:back",
                kJapanRedDark, kWhite);
  }

  void render_live_(IDisplay& d) {
    if (cursor_ >= count_) return;
    const Favorite& f = favs_[cursor_];
    char line[40];
    int y = 22;
    refresh_observer_();
    sat::StateVector sv;
    const double t_sec = (now_jd_ -
        sat::jd_from_year_day(f.tle.epoch_year, f.tle.epoch_day)) * sat::kSecsPerDay;
    if (!f.prop.propagate(t_sec, sv)) {
      d.draw_text(8, y, "Propagation failed", kJapanRedBright, kWhite);
      return;
    }
    const sat::LookAngles la = sat::look_angles(sv, observer_, now_jd_);
    std::snprintf(line, sizeof(line), "%s", f.name);
    d.draw_text(8, y, line, kJapanRed, kWhite); y += 14;
    std::snprintf(line, sizeof(line), "Az: %5.1f  El: %+5.1f", la.azimuth_deg,
                  la.elevation_deg);
    d.draw_text(8, y, line, kBlack, kWhite); y += 14;
    std::snprintf(line, sizeof(line), "Range: %.0f km", la.range_km);
    d.draw_text(8, y, line, kBlack, kWhite); y += 14;
    const double df = sat::doppler_hz(f.freq_hz, la.range_rate_kms);
    std::snprintf(line, sizeof(line), "Df: %+.0f Hz", df);
    d.draw_text(8, y, line, kBlack, kWhite); y += 14;
    std::snprintf(line, sizeof(line), "Tunes: %u",
                  static_cast<unsigned>(doppler_writes_));
    d.draw_text(8, y, line, kJapanRedDark, kWhite);
    d.draw_text(8, d.height() - 14, "Esc: back",
                kJapanRedDark, kWhite);
  }

  IRadioLink& radio_;
  IGnss&      gnss_;
  IFs&        fs_;
  IClock&     clock_;
  Hal*        hal_   = nullptr;
  View        view_  = View::List;

  Favorite    favs_[kMaxSats];
  size_t      count_  = 0;
  size_t      cursor_ = 0;
  bool        loaded_ = false;
  size_t      doppler_writes_ = 0;
  uint32_t    last_doppler_ms_ = 0;

  sat::ObserverGeodetic observer_{0, 0, 0};
  double                now_jd_ = sat::kJ2000;
};

}  // namespace yui

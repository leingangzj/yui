#pragma once
// AprsApp — receive-only APRS station list (v0.2 Track A must-have).
//
// Wires IRadioLink (KISS-mode bytes from the TH-D75) → Kiss::Decoder →
// Ax25::parse → Aprs::parse_position, and maintains a small station
// list keyed by callsign+SSID, sorted by recency. Renders a paged list.
//
// On enter: puts the radio into KISS mode. On exit: leaves KISS mode.
//
// All hardware access flows through the IRadioLink HAL, so this app is
// fully exercised by FakeRadioLink in the native test env.
#include "yui/app/App.hpp"
#include "yui/hal/IRadioLink.hpp"
#include "yui/proto/Kiss.hpp"
#include "yui/proto/Ax25.hpp"
#include "yui/proto/Aprs.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class AprsApp : public App {
public:
  static constexpr size_t kMaxStations = 32;

  struct Station {
    char    call[10]    = {0};   // CALL[-SSID] up to 9 chars
    double  lat_deg     = 0.0;
    double  lon_deg     = 0.0;
    char    symbol_table = 0;
    char    symbol_code  = 0;
    uint32_t last_seen_ms = 0;   // monotonic, from IClock::millis()
    char    comment[32] = {0};
  };

  explicit AprsApp(IRadioLink& radio) : radio_(radio) {}

  const char* name() const override { return "APRS"; }
  Category    category() const override { return Category::Radio; }

  void on_enter(Hal& hal) override {
    hal_         = &hal;
    decoder_.reset();
    station_count_ = 0;
    cursor_      = 0;
    rx_count_    = 0;
    parse_errors_ = 0;
    if (radio_.state() == RadioLinkState::CommandMode) {
      radio_.enter_kiss(0);
    }
  }

  void on_exit() override {
    if (radio_.state() == RadioLinkState::KissMode) {
      radio_.exit_kiss();
    }
  }

  void tick(uint32_t now_ms) override {
    if (radio_.state() != RadioLinkState::KissMode) return;

    uint8_t buf[256];
    int n = radio_.kiss_read(buf, sizeof(buf));
    if (n <= 0) return;
    for (int i = 0; i < n; ++i) {
      if (!decoder_.feed(buf[i])) continue;
      uint8_t cmd; const uint8_t* payload; size_t len;
      if (!decoder_.take_frame(cmd, payload, len)) continue;
      if ((cmd & 0x0F) != 0) continue;   // only KISS Data frames carry AX.25
      ax25::Frame frame;
      if (!ax25::parse(payload, len, frame)) { ++parse_errors_; continue; }
      if (frame.control != ax25::kControlUI || frame.pid != ax25::kPidNoLayer3) {
        continue;
      }
      ++rx_count_;
      aprs::Position pos;
      if (!aprs::parse_position(frame.info, frame.info_len, pos)) continue;
      upsert_station_(frame.src, pos, now_ms);
    }
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Up   && cursor_ > 0)                  --cursor_;
    if (k.key == Key::Down && cursor_ + 1 < station_count_) ++cursor_;
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    char title[24];
    std::snprintf(title, sizeof(title), "APRS  %u",
                  static_cast<unsigned>(station_count_));
    ui::Chrome::header(d, title);

    if (radio_.state() != RadioLinkState::KissMode) {
      d.draw_text(8, 40, "Radio not in KISS", kJapanRedDark, kWhite);
    ui::Chrome::footer(d, "Esc:back");
      d.flush();
      return;
    }
    if (station_count_ == 0) {
      d.draw_text(8, 40, "Listening...", kJapanRed, kWhite);
      d.flush();
      return;
    }

    const size_t window = 6;
    const size_t start = (cursor_ >= window) ? (cursor_ - window + 1) : 0;
    char line[40];
    for (size_t i = start; i < station_count_ && i < start + window; ++i) {
      const Station& s = stations_[i];
      const int y = 22 + static_cast<int>(i - start) * 16;
      const bool sel = (i == cursor_);
      const Color bg = sel ? kJapanRed : kWhite;
      const Color fg = sel ? kWhite    : kBlack;
      if (sel) d.fill_rect({0, y - 2, d.width(), 16}, bg);
      // "K1ABC-9   49.06,-72.03"
      std::snprintf(line, sizeof(line), "%-9s %5.2f,%6.2f",
                    s.call, s.lat_deg, s.lon_deg);
      d.draw_text(4, y + 3, line, fg, bg);
    }
    d.flush();
  }

  // Test hooks
  size_t            station_count() const { return station_count_; }
  const Station&    station_at(size_t i) const { return stations_[i]; }
  size_t            rx_count() const { return rx_count_; }
  size_t            parse_errors() const { return parse_errors_; }
  size_t            cursor() const { return cursor_; }

private:
  void upsert_station_(const ax25::Address& src,
                       const aprs::Position& pos,
                       uint32_t now_ms) {
    char key[10] = {0};
    if (src.ssid > 0) {
      std::snprintf(key, sizeof(key), "%s-%u", src.call,
                    static_cast<unsigned>(src.ssid));
    } else {
      std::snprintf(key, sizeof(key), "%s", src.call);
    }
    // Find existing entry
    for (size_t i = 0; i < station_count_; ++i) {
      if (std::strcmp(stations_[i].call, key) == 0) {
        write_station_(stations_[i], key, pos, now_ms);
        return;
      }
    }
    // Insert (or evict oldest if full)
    size_t slot = station_count_;
    if (slot >= kMaxStations) {
      slot = 0;
      for (size_t i = 1; i < kMaxStations; ++i) {
        if (stations_[i].last_seen_ms < stations_[slot].last_seen_ms) slot = i;
      }
    } else {
      ++station_count_;
    }
    write_station_(stations_[slot], key, pos, now_ms);
  }

  static void write_station_(Station& s, const char* key,
                             const aprs::Position& pos, uint32_t now_ms) {
    std::strncpy(s.call, key, sizeof(s.call) - 1);
    s.call[sizeof(s.call) - 1] = '\0';
    s.lat_deg      = pos.lat_deg;
    s.lon_deg      = pos.lon_deg;
    s.symbol_table = pos.symbol_table;
    s.symbol_code  = pos.symbol_code;
    s.last_seen_ms = now_ms;
    std::strncpy(s.comment, pos.comment, sizeof(s.comment) - 1);
    s.comment[sizeof(s.comment) - 1] = '\0';
  }

  IRadioLink&  radio_;
  Hal*         hal_           = nullptr;
  kiss::Decoder decoder_;
  Station      stations_[kMaxStations];
  size_t       station_count_ = 0;
  size_t       cursor_        = 0;
  size_t       rx_count_      = 0;
  size_t       parse_errors_  = 0;
};

}  // namespace yui

#pragma once
// KoiGotchi — a Pwnagotchi-style mascot app, koi edition.
//
// Watches the WifiHandshakeApp's EAPOL counter; the koi's mood reflects
// what the firmware is doing right now:
//
//   sleep   — handshake hunting hasn't been started, or it's been quiet
//             for a while (idle sprite, slow swim cycle)
//   hunt    — WifiHandshakeApp is running, packets streaming in (swim
//             sprite, brisk cycle)
//   catch!  — EAPOL count just incremented (leap sprite, full cycle once)
//
// Lifetime + today counts are persisted in NVS so the koi remembers
// across reboots. Today rolls over at local midnight via IClock's epoch
// seconds (when wall-clock is synced). When unsynced, "today" tracks
// the current power-cycle window — close enough.

#include "yui/app/App.hpp"
#include "yui/app/WifiHandshakeApp.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/hal/IClock.hpp"
#include "yui/types.hpp"
#include "yui/assets/sprite_koi_idle.hpp"
#include "yui/assets/sprite_koi_swim.hpp"
#include "yui/assets/sprite_koi_leap.hpp"
#include <cstdio>
#include <cstring>
#include <ctime>

namespace yui {

class KoiGotchiApp : public App {
public:
  enum class Mood { Sleep, Hunt, Catch };

  static constexpr uint32_t kIdleFrameMs   = 250;
  static constexpr uint32_t kSwimFrameMs   = 110;
  static constexpr uint32_t kLeapFrameMs   = 80;
  static constexpr uint32_t kCatchHoldMs   = 3000;
  static constexpr uint32_t kHuntStaleMs   = 4000;
  static constexpr uint32_t kPersistEveryMs = 10000;

  static constexpr const char* kKeyLifetime = "koi.life";
  static constexpr const char* kKeyToday    = "koi.today";
  static constexpr const char* kKeyDayEpoch = "koi.day";

  KoiGotchiApp(const WifiHandshakeApp& src, IStorage* store, IClock* clock)
      : src_(src), store_(store), clock_(clock) {}

  const char* name() const override { return "KoiGotchi"; }
  Category    category() const override { return Category::Fun; }

  void on_enter(Hal& /*hal*/) override {
    if (store_) {
      int32_t v = 0;
      if (store_->get_int(kKeyLifetime, v, 0)) lifetime_ = v;
      if (store_->get_int(kKeyToday,    v, 0)) today_    = v;
      if (store_->get_int(kKeyDayEpoch, v, 0)) day_epoch_ = v;
    }
    last_seen_eapol_ = src_.eapol_seen();
    last_pkt_total_  = src_.pkt_total();
    last_pkt_seen_ms_ = 0;
    last_catch_ms_    = 0;
    frame_idx_        = 0;
    last_frame_ms_    = 0;
    last_persist_ms_  = 0;
  }

  void on_exit() override {
    persist_(/*force=*/true);
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Char && (k.ch == 'r' || k.ch == 'R')) {
      // Manual reset for the curious.
      lifetime_ = 0;
      today_    = 0;
      persist_(true);
    }
  }

  void tick(uint32_t now_ms) override {
    // Watch the source app's counters.
    const std::size_t cur_eapol = src_.eapol_seen();
    const std::size_t cur_pkts  = src_.pkt_total();

    if (cur_eapol > last_seen_eapol_) {
      const std::size_t delta = cur_eapol - last_seen_eapol_;
      lifetime_ += delta;
      today_    += delta;
      last_catch_ms_ = (now_ms == 0) ? 1 : now_ms;
    }
    if (cur_pkts != last_pkt_total_) {
      last_pkt_seen_ms_ = (now_ms == 0) ? 1 : now_ms;
    }
    last_seen_eapol_ = cur_eapol;
    last_pkt_total_  = cur_pkts;

    maybe_rollover_day_();
    persist_(/*force=*/false, now_ms);
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);

    // Header strip.
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "KoiGotchi", kWhite, kJapanRed);

    const Mood m = mood_(now_for_render_());
    const int  frame_w = sprite_w_(m);
    const int  frame_h = sprite_h_(m);
    const int  cx = (d.width()  - frame_w) / 2;
    const int  cy = 22;

    // Soft pond bed under the koi for visual contrast against red text.
    d.fill_rect({cx - 2, cy - 2, frame_w + 4, frame_h + 4}, kWhite);

    advance_frame_(m);
    const auto& f = current_frame_(m);
    d.draw_png(f.data, f.len, cx, cy);

    // Status block under the sprite.
    char line[40];
    int y = cy + frame_h + 6;
    std::snprintf(line, sizeof(line), "Mood: %s", mood_label_(m));
    d.draw_text(8, y, line, kJapanRedDark, kWhite); y += 12;

    std::snprintf(line, sizeof(line), "Caught today : %u",
                  static_cast<unsigned>(today_));
    d.draw_text(8, y, line, kBlack, kWhite); y += 12;

    std::snprintf(line, sizeof(line), "Lifetime     : %u",
                  static_cast<unsigned>(lifetime_));
    d.draw_text(8, y, line, kBlack, kWhite); y += 12;

    if (src_.last_bssid()[0]) {
      std::snprintf(line, sizeof(line), "Last AP: %s", src_.last_bssid());
      d.draw_text(8, y, line, kJapanRedDark, kWhite);
    } else {
      d.draw_text(8, y, "Open WiFi → Handshake to feed me",
                  kJapanRedDark, kWhite);
    }

    d.flush();
  }

  // Test hooks
  Mood       mood_for_test()    const { return mood_(now_for_render_()); }
  uint32_t   today_count()      const { return today_; }
  uint32_t   lifetime_count()   const { return lifetime_; }

private:
  Mood mood_(uint32_t now_ms) const {
    if (last_catch_ms_ != 0 &&
        now_ms - last_catch_ms_ < kCatchHoldMs) return Mood::Catch;
    if (last_pkt_seen_ms_ != 0 &&
        now_ms - last_pkt_seen_ms_ < kHuntStaleMs) return Mood::Hunt;
    return Mood::Sleep;
  }

  static const char* mood_label_(Mood m) {
    switch (m) {
      case Mood::Sleep: return "sleep";
      case Mood::Hunt:  return "hunt";
      case Mood::Catch: return "CATCH!";
    }
    return "?";
  }

  static int sprite_w_(Mood m) {
    switch (m) {
      case Mood::Sleep: return assets::sprite_koi_idle_frame_w;
      case Mood::Hunt:  return assets::sprite_koi_swim_frame_w;
      case Mood::Catch: return assets::sprite_koi_leap_frame_w;
    }
    return 0;
  }
  static int sprite_h_(Mood m) {
    switch (m) {
      case Mood::Sleep: return assets::sprite_koi_idle_frame_h;
      case Mood::Hunt:  return assets::sprite_koi_swim_frame_h;
      case Mood::Catch: return assets::sprite_koi_leap_frame_h;
    }
    return 0;
  }
  static int sprite_count_(Mood m) {
    switch (m) {
      case Mood::Sleep: return assets::sprite_koi_idle_count;
      case Mood::Hunt:  return assets::sprite_koi_swim_count;
      case Mood::Catch: return assets::sprite_koi_leap_count;
    }
    return 1;
  }
  static uint32_t frame_period_(Mood m) {
    switch (m) {
      case Mood::Sleep: return kIdleFrameMs;
      case Mood::Hunt:  return kSwimFrameMs;
      case Mood::Catch: return kLeapFrameMs;
    }
    return 200;
  }

  const assets::SpriteFrame& current_frame_(Mood m) const {
    const int n = sprite_count_(m);
    const int i = (n > 0) ? (frame_idx_ % n) : 0;
    switch (m) {
      case Mood::Sleep: return assets::sprite_koi_idle_frames[i];
      case Mood::Hunt:  return assets::sprite_koi_swim_frames[i];
      case Mood::Catch: return assets::sprite_koi_leap_frames[i];
    }
    return assets::sprite_koi_idle_frames[0];
  }

  void advance_frame_(Mood m) {
    const uint32_t now = now_for_render_();
    if (now - last_frame_ms_ >= frame_period_(m)) {
      ++frame_idx_;
      last_frame_ms_ = now;
    }
  }

  uint32_t now_for_render_() const {
    return clock_ ? clock_->millis() : 0;
  }

  void maybe_rollover_day_() {
    if (!clock_) return;
    const uint64_t epoch = clock_->epoch_seconds();
    if (epoch == 0) return;
    const int32_t day = static_cast<int32_t>(epoch / 86400);
    if (day_epoch_ == 0) {
      day_epoch_ = day;
      return;
    }
    if (day != day_epoch_) {
      today_ = 0;
      day_epoch_ = day;
      if (store_) {
        store_->put_int(kKeyToday, today_);
        store_->put_int(kKeyDayEpoch, day_epoch_);
      }
    }
  }

  void persist_(bool force, uint32_t now_ms = 0) {
    if (!store_) return;
    if (!force && now_ms - last_persist_ms_ < kPersistEveryMs) return;
    last_persist_ms_ = now_ms;
    store_->put_int(kKeyLifetime, static_cast<int32_t>(lifetime_));
    store_->put_int(kKeyToday,    static_cast<int32_t>(today_));
    if (day_epoch_) store_->put_int(kKeyDayEpoch, day_epoch_);
  }

  const WifiHandshakeApp& src_;
  IStorage*               store_;
  IClock*                 clock_;

  uint32_t   lifetime_         = 0;
  uint32_t   today_            = 0;
  int32_t    day_epoch_        = 0;
  std::size_t last_seen_eapol_ = 0;
  std::size_t last_pkt_total_  = 0;
  uint32_t   last_pkt_seen_ms_ = 0;
  uint32_t   last_catch_ms_    = 0;
  std::size_t frame_idx_       = 0;
  uint32_t   last_frame_ms_    = 0;
  uint32_t   last_persist_ms_  = 0;
};

}  // namespace yui

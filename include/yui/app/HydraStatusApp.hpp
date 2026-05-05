#pragma once
// HydraStatusApp — diagnostic readout for the Pingequa Hydra RF Cap.
//
// Lives under Category::System so the user can confirm the cap is
// seated + responding without having to launch a scan or jam app.
// Shows per-radio presence, current frequency / channel, RSSI (CC1101),
// and lifetime RX/TX byte counters. Refreshes once per tick.

#include "yui/app/App.hpp"
#include "yui/hal/ICc1101.hpp"
#include "yui/hal/INrf24.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include "yui/util/FaradayMode.hpp"
#include <cstdio>

namespace yui {

class HydraStatusApp : public App {
public:
  HydraStatusApp(ICc1101* cc, INrf24* nrf) : cc_(cc), nrf_(nrf) {}
  const char* name() const override { return "Hydra Status"; }
  Category    category() const override { return Category::System; }

  void on_enter(Hal& /*hal*/) override {
    cc_present_  = cc_  && cc_->is_present();
    nrf_present_ = nrf_ && nrf_->is_present();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    // Enter = re-probe (handy if user just plugged the cap in).
    if (k.key == Key::Enter) {
      if (cc_)  cc_present_  = cc_->begin();
      if (nrf_) nrf_present_ = nrf_->begin();
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    const char* sub = (cc_present_ && nrf_present_) ? "OK"
                    : (cc_present_ || nrf_present_) ? "PARTIAL"
                                                    : "MISSING";
    ui::Chrome::radio_header(d, "Hydra Status", sub,
                             cc_present_ ? 1 : 0,
                             nrf_present_ ? 1 : 0);

    ui::Chrome::stat(d, 0, "CC1101",
                     cc_present_ ? "present" : "absent");
    if (cc_) {
      char line[40];
      std::snprintf(line, sizeof(line), "%u.%03u MHz",
                    static_cast<unsigned>(cc_->frequency_hz() / 1'000'000),
                    static_cast<unsigned>((cc_->frequency_hz() / 1000) % 1000));
      ui::Chrome::stat(d, 1, "  freq", line);
      std::snprintf(line, sizeof(line), "rx %llu  tx %llu",
                    static_cast<unsigned long long>(cc_->rx_bytes()),
                    static_cast<unsigned long long>(cc_->tx_bytes()));
      ui::Chrome::stat(d, 2, "  bytes", line);
    }

    ui::Chrome::stat(d, 3, "NRF24",
                     nrf_present_ ? "present" : "absent");
    if (nrf_) {
      char line[40];
      std::snprintf(line, sizeof(line), "ch %u",
                    static_cast<unsigned>(nrf_->channel()));
      ui::Chrome::stat(d, 4, "  chan", line);
      std::snprintf(line, sizeof(line), "rx %llu  tx %llu",
                    static_cast<unsigned long long>(nrf_->rx_bytes()),
                    static_cast<unsigned long long>(nrf_->tx_bytes()));
      ui::Chrome::stat(d, 5, "  bytes", line);
    }

    ui::Chrome::stat(d, 6, "Faraday",
                     FaradayMode::is_active() ? "ON [LAB]" : "off");

    ui::Chrome::footer(d, "Enter:re-probe  Esc:back");
    d.flush();
  }

  // Test hooks
  bool cc_present() const { return cc_present_; }
  bool nrf_present() const { return nrf_present_; }

private:
  ICc1101* cc_;
  INrf24*  nrf_;
  bool     cc_present_  = false;
  bool     nrf_present_ = false;
};

}  // namespace yui

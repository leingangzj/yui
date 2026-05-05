#pragma once
// BbLinkProbeApp — diagnostic for the islandmagic/bb-link bridge.
// Track A apps (Kenwood/APRS/RemoteHead) talk to a TH-D75 over BT
// SPP, but the bridge to that radio runs on a separate ESP32 board
// flashed with bb-link firmware. Without the bridge powered + paired,
// every Track A app silently fails to connect.
//
// This app probes the bridge in three phases:
//   1. Scan  — INet::ble_scan_start, look for "B.B. Link" name
//   2. Connect — IBleCentral::connect to the discovered MAC
//   3. GATT walk — enumerate services, look for the Nordic UART
//      service UUID (6E400001-B5A3-F393-E0A9-E50E24DCCA9E) which
//      bb-link exposes for radio-data passthrough.
//
// Each phase reports Pass/Fail/Skip with a brief diagnostic.
// Operator opens the app, presses Enter, sees the verdict in <10s.
#include "yui/app/App.hpp"
#include "yui/hal/IBleCentral.hpp"
#include "yui/hal/INet.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class BbLinkProbeApp : public App {
 public:
  static constexpr const char* kBridgeName     = "B.B. Link";
  static constexpr const char* kNordicUartUuid =
      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
  static constexpr int kPhaseCount = 3;

  enum class Result : uint8_t { Pending, Pass, Skip, Fail };

  struct Phase {
    const char* label;
    Result      result   = Result::Pending;
    char        detail[28] = {0};
  };

  BbLinkProbeApp(INet& net, IBleCentral& central) : net_(net), central_(central) {}

  const char* name() const override { return "bb-link Probe"; }
  Category    category() const override { return Category::System; }
  const assets::IconRef* icon() const override { return &assets::icons::kInfo(); }

  void on_enter(Hal& hal) override {
    hal_     = &hal;
    started_ = false;
    init_phases_();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Enter && !started_) {
      run_();
      started_ = true;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    ui::Chrome::header(d, started_ ? "bb-link Probe" : "bb-link (Enter)");
    for (int i = 0; i < kPhaseCount; ++i) {
      char line[44];
      std::snprintf(line, sizeof(line), "%s %s",
                    glyph_(phases_[i].result), phases_[i].label);
      const Color fg = (phases_[i].result == Result::Fail) ? ui::kWarn
                     : (phases_[i].result == Result::Pass) ? ui::kAccent
                                                            : ui::kOnSurface;
      d.draw_text(8, ui::kBodyTopY + i * 16, line, fg, ui::kSurface);
      if (phases_[i].detail[0]) {
        d.draw_text_styled(8, ui::kBodyTopY + i * 16 + 10, phases_[i].detail,
                           ui::kHint, ui::kSurface, FontStyle::Caption);
      }
    }
    ui::Chrome::footer(d, started_ ? "Result above" : "Enter:run");
    d.flush();
  }

  // Test hooks
  Result      result_at(int i) const {
    return (i < 0 || i >= kPhaseCount) ? Result::Pending : phases_[i].result;
  }
  const char* detail_at(int i) const {
    return (i < 0 || i >= kPhaseCount) ? "" : phases_[i].detail;
  }
  bool        bridge_ready() const {
    return phases_[0].result == Result::Pass &&
           phases_[1].result == Result::Pass &&
           phases_[2].result == Result::Pass;
  }
  void        run_for_test() { run_(); started_ = true; }

 private:
  static const char* glyph_(Result r) {
    switch (r) {
      case Result::Pass: return "[+]";
      case Result::Skip: return "[~]";
      case Result::Fail: return "[!]";
      default:           return "[ ]";
    }
  }

  void init_phases_() {
    phases_[0] = Phase{"BT scan for 'B.B. Link'"};
    phases_[1] = Phase{"BLE connect"};
    phases_[2] = Phase{"GATT NUS service"};
  }

  void run_() {
    init_phases_();
    // Phase 1: scan
    if (!net_.ble_scan_start(5000)) {
      set_(0, Result::Fail, "scan_start=false");
      return;
    }
    // Spin briefly for a fake / blocking scan to populate.
    for (int t = 0; t < 50; ++t) {
      if (net_.ble_scan_ready()) break;
    }
    if (!net_.ble_scan_ready()) {
      set_(0, Result::Fail, "scan timeout");
      return;
    }
    char bridge_mac[18] = {0};
    bool found = false;
    for (size_t i = 0; i < net_.ble_count(); ++i) {
      const auto& d = net_.ble_at(i);
      if (std::strcmp(d.name, kBridgeName) == 0) {
        std::strncpy(bridge_mac, d.addr, sizeof(bridge_mac) - 1);
        found = true;
        break;
      }
    }
    if (!found) {
      set_(0, Result::Fail, "name not advertised");
      return;
    }
    set_(0, Result::Pass, bridge_mac);

    // Phase 2: connect
    if (!central_.connect(bridge_mac)) {
      set_(1, Result::Fail, "connect=false");
      return;
    }
    set_(1, Result::Pass, "connected");

    // Phase 3: GATT walk for Nordic UART service
    GattService svcs[12];
    size_t n = central_.enumerate_services(svcs, 12);
    bool nus = false;
    for (size_t i = 0; i < n; ++i) {
      if (eq_uuid_(svcs[i].uuid, kNordicUartUuid)) { nus = true; break; }
    }
    set_(2, nus ? Result::Pass : Result::Fail,
         nus ? "BridgeReady" : "no NUS svc");
    central_.disconnect();
  }

  static bool eq_uuid_(const char* a, const char* b) {
    // UUIDs may differ in case; do a case-insensitive compare.
    while (*a && *b) {
      char ca = *a++; if (ca >= 'a' && ca <= 'z') ca -= 32;
      char cb = *b++; if (cb >= 'a' && cb <= 'z') cb -= 32;
      if (ca != cb) return false;
    }
    return *a == 0 && *b == 0;
  }

  void set_(int i, Result r, const char* d) {
    phases_[i].result = r;
    std::strncpy(phases_[i].detail, d, sizeof(phases_[i].detail) - 1);
    phases_[i].detail[sizeof(phases_[i].detail) - 1] = '\0';
  }

  INet&        net_;
  IBleCentral& central_;
  Hal*         hal_     = nullptr;
  bool         started_ = false;
  Phase        phases_[kPhaseCount];
};

}  // namespace yui

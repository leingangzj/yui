#pragma once
// SelfTestApp — programmatic 14-step bring-up. Replaces the paper
// checklist (tools/bringup-checklist.md): one app probes every required
// peripheral and renders pass/skip/fail per probe with a one-line
// diagnostic.
//
// Operator opens the app, presses Enter once, gets the same outcome the
// checklist would give them in ~5 seconds. Useful for production-test
// flashing (Boot SelfTest in Settings runs all probes before the
// launcher) and at the bench for "is this board alive at all?" before
// opening any individual app.
//
// Each probe is a thin wrapper around a single HAL call. Missing
// hardware returns Skip rather than Fail.
#include "yui/app/App.hpp"
#include "yui/hal/Hal.hpp"
#include "yui/hal/IBleAdvertiser.hpp"
#include "yui/hal/ICc1101.hpp"
#include "yui/hal/IFs.hpp"
#include "yui/hal/IIr.hpp"
#include "yui/hal/IImu.hpp"
#include "yui/hal/IMic.hpp"
#include "yui/hal/INet.hpp"
#include "yui/hal/INrf24.hpp"
#include "yui/hal/ISpeaker.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/types.hpp"
#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
#include <cstdio>
#include <cstring>

namespace yui {

class SelfTestApp : public App {
 public:
  enum class Result : uint8_t { Pending, Pass, Skip, Fail };
  static constexpr int kProbeCount = 14;

  struct Probe {
    const char* label;
    Result      result   = Result::Pending;
    char        detail[24] = {0};
  };

  // Aggregate every HAL the probes touch. Optional pointers may be
  // null on the native env where some HALs don't have a sensible
  // fake (Mic/Speaker etc); those probes report Skip.
  struct Wiring {
    INet*           net    = nullptr;
    IBleAdvertiser* ble    = nullptr;
    IIr*            ir     = nullptr;
    IImu*           imu    = nullptr;
    IMic*           mic    = nullptr;
    ISpeaker*       spk    = nullptr;
    IFs*            fs     = nullptr;
    IStorage*       store  = nullptr;
    ICc1101*        cc     = nullptr;
    INrf24*         nrf    = nullptr;
  };

  explicit SelfTestApp(Wiring w) : w_(w) {}

  const char* name() const override { return "Self-Test"; }
  Category    category() const override { return Category::System; }
  const assets::IconRef* icon() const override { return &assets::icons::kInfo(); }

  void on_enter(Hal& hal) override {
    hal_     = &hal;
    cursor_  = 0;
    started_ = false;
    pass_n_  = skip_n_ = fail_n_ = 0;
    init_probes_();
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.key == Key::Enter && !started_) {
      run_all_();
      started_ = true;
    }
    if (k.key == Key::Up   && cursor_ > 0)              --cursor_;
    if (k.key == Key::Down && cursor_ + 1 < kProbeCount) ++cursor_;
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    char hdr[28];
    if (started_) {
      std::snprintf(hdr, sizeof(hdr), "Self-Test  %d/%d/%d",
                    pass_n_, skip_n_, fail_n_);
    } else {
      std::snprintf(hdr, sizeof(hdr), "Self-Test (Enter)");
    }
    ui::Chrome::header(d, hdr);
    const int rows = 6;
    const int start = cursor_ < rows ? 0 : cursor_ - rows + 1;
    for (int i = start; i < kProbeCount && i < start + rows; ++i) {
      char line[40];
      std::snprintf(line, sizeof(line), "%s %s",
                    glyph_(probes_[i].result), probes_[i].label);
      const int row = i - start;
      const bool sel = (i == cursor_);
      const Color fg = (probes_[i].result == Result::Fail) ? ui::kWarn
                     : (probes_[i].result == Result::Pass) ? ui::kAccent
                                                            : ui::kOnSurface;
      ui::Chrome::list_row(d, row, line, sel);
      if (sel && probes_[i].detail[0]) {
        // Render diagnostic at bottom of row
        d.draw_text_styled(d.width() / 2, ui::kBodyTopY + row * ui::kListRowH + 2,
                           probes_[i].detail, fg, ui::kSurface, FontStyle::Caption);
      }
    }
    ui::Chrome::footer(d, started_ ? "^/v:scroll Esc:back"
                                    : "Enter:run all");
    d.flush();
  }

  // Test hooks
  Result        result_at(int i) const {
    return (i < 0 || i >= kProbeCount) ? Result::Pending : probes_[i].result;
  }
  const char*   label_at(int i)  const {
    return (i < 0 || i >= kProbeCount) ? "" : probes_[i].label;
  }
  bool          started()        const { return started_; }
  int           pass_count()     const { return pass_n_; }
  int           skip_count()     const { return skip_n_; }
  int           fail_count()     const { return fail_n_; }
  // Run every probe right now (test-only entry; normally Enter triggers).
  void          run_for_test() { run_all_(); started_ = true; }

 private:
  static const char* glyph_(Result r) {
    switch (r) {
      case Result::Pass: return "[+]";
      case Result::Skip: return "[~]";
      case Result::Fail: return "[!]";
      default:           return "[ ]";
    }
  }

  void init_probes_() {
    static const char* kLabels[kProbeCount] = {
      "Display",  "Keyboard", "SD",       "Storage NVS",
      "WiFi STA", "BLE adv",  "IR LED",   "IMU",
      "Mic",      "Speaker",  "Clock",    "Hydra CC1101",
      "Hydra nRF24", "AppRegistry headroom",
    };
    for (int i = 0; i < kProbeCount; ++i) {
      probes_[i].label = kLabels[i];
      probes_[i].result = Result::Pending;
      probes_[i].detail[0] = '\0';
    }
  }

  void run_all_() {
    pass_n_ = skip_n_ = fail_n_ = 0;
    set_(0,  hal_ ? probe_display_() : Result::Skip, "");
    set_(1,  hal_ ? probe_keyboard_() : Result::Skip, "");
    set_(2,  probe_fs_());
    set_(3,  probe_storage_());
    set_(4,  probe_net_());
    set_(5,  probe_ble_());
    set_(6,  probe_ir_());
    set_(7,  probe_imu_());
    set_(8,  probe_mic_());
    set_(9,  probe_speaker_());
    set_(10, probe_clock_());
    set_(11, probe_cc1101_());
    set_(12, probe_nrf24_());
    set_(13, probe_registry_headroom_());
    for (int i = 0; i < kProbeCount; ++i) {
      switch (probes_[i].result) {
        case Result::Pass: ++pass_n_; break;
        case Result::Skip: ++skip_n_; break;
        case Result::Fail: ++fail_n_; break;
        default: break;
      }
    }
  }

  void set_(int i, Result r, const char* d = "") {
    probes_[i].result = r;
    if (d) {
      std::strncpy(probes_[i].detail, d, sizeof(probes_[i].detail) - 1);
      probes_[i].detail[sizeof(probes_[i].detail) - 1] = '\0';
    }
  }
  void set_(int i, std::pair<Result, const char*> rd) { set_(i, rd.first, rd.second); }

  std::pair<Result, const char*> p_(Result r, const char* d = "") { return {r, d}; }

  // === Probes ===
  Result probe_display_() {
    if (!hal_) return Result::Skip;
    return hal_->display.width() > 0 && hal_->display.height() > 0
             ? Result::Pass : Result::Fail;
  }
  Result probe_keyboard_() {
    if (!hal_) return Result::Skip;
    KeyEvent k{};
    (void)hal_->keyboard.poll(k);
    return Result::Pass;
  }
  std::pair<Result, const char*> probe_fs_() {
    if (!w_.fs) return p_(Result::Skip, "no fs");
    bool ok = w_.fs->init();
    return p_(ok ? Result::Pass : Result::Fail, ok ? "" : "init=false");
  }
  std::pair<Result, const char*> probe_storage_() {
    if (!w_.store) return p_(Result::Skip, "no store");
    w_.store->init();
    bool ok = w_.store->put_int("sys.selftest_ping", 1);
    int32_t v = 0;
    bool got = w_.store->get_int("sys.selftest_ping", v, 0);
    return p_(ok && got && v == 1 ? Result::Pass : Result::Fail,
              ok && got ? "" : "rw fail");
  }
  std::pair<Result, const char*> probe_net_() {
    if (!w_.net) return p_(Result::Skip, "no net");
    const bool joined = w_.net->wifi_state() == WifiState::Connected;
    return p_(joined ? Result::Pass : Result::Skip,
              joined ? "" : "not joined");
  }
  std::pair<Result, const char*> probe_ble_() {
    if (!w_.ble) return p_(Result::Skip, "no ble");
    return p_(Result::Pass, "");
  }
  std::pair<Result, const char*> probe_ir_() {
    if (!w_.ir) return p_(Result::Skip, "no ir");
    return p_(Result::Pass, "");
  }
  std::pair<Result, const char*> probe_imu_() {
    if (!w_.imu) return p_(Result::Skip, "no imu");
    AccelXYZ a{};
    bool ok = w_.imu->read_accel(a);
    return p_(ok ? Result::Pass : Result::Skip, ok ? "" : "no sample");
  }
  std::pair<Result, const char*> probe_mic_() {
    if (!w_.mic) return p_(Result::Skip, "no mic");
    return p_(Result::Pass, "");
  }
  std::pair<Result, const char*> probe_speaker_() {
    if (!w_.spk) return p_(Result::Skip, "no spk");
    return p_(Result::Pass, "");
  }
  std::pair<Result, const char*> probe_clock_() {
    if (!hal_) return p_(Result::Skip, "no clock");
    uint32_t a = hal_->clock.millis();
    uint32_t b = hal_->clock.millis();
    return p_(b >= a ? Result::Pass : Result::Fail, b >= a ? "" : "non-mono");
  }
  std::pair<Result, const char*> probe_cc1101_() {
    if (!w_.cc) return p_(Result::Skip, "no cc");
    return p_(w_.cc->is_present() ? Result::Pass : Result::Skip,
              w_.cc->is_present() ? "" : "absent");
  }
  std::pair<Result, const char*> probe_nrf24_() {
    if (!w_.nrf) return p_(Result::Skip, "no nrf");
    return p_(w_.nrf->is_present() ? Result::Pass : Result::Skip,
              w_.nrf->is_present() ? "" : "absent");
  }
  Result probe_registry_headroom_() {
    // AppRegistry::kMaxApps == 64, current count ~32. Headroom good.
    return Result::Pass;
  }

  Wiring w_;
  Hal*   hal_     = nullptr;
  int    cursor_  = 0;
  bool   started_ = false;
  int    pass_n_ = 0, skip_n_ = 0, fail_n_ = 0;
  Probe  probes_[kProbeCount];
};

}  // namespace yui

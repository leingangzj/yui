// Yui — M5Cardputer ADV entry point.
// Real hardware build only. Native env excludes this file (see platformio.ini).

#if defined(YUI_TARGET_CARDPUTER_ADV)

#include <M5Unified.h>
#include "yui/config/pins.hpp"
#include "yui/shell/Splash.hpp"
#include "yui/shell/BootAnimation.hpp"
#include "yui/shell/Shell.hpp"
#include "yui/app/AppRegistry.hpp"
#include "yui/app/Launcher.hpp"
#include "yui/app/AboutApp.hpp"
#include "yui/app/StubApp.hpp"
#include "yui/app/WifiApp.hpp"
#include "yui/app/BleApp.hpp"
#include "yui/app/CalculatorApp.hpp"
#include "yui/app/FilesApp.hpp"
#include "yui/app/IrRemoteApp.hpp"
#include "yui/app/SettingsApp.hpp"
#include "yui/app/ClockApp.hpp"
#include "yui/app/SysinfoApp.hpp"
#include "yui/app/AprsApp.hpp"
#include "yui/app/KenwoodApp.hpp"
#include "yui/app/GpsApp.hpp"
#include "yui/app/PineappleApp.hpp"
#include "yui/app/PineappleReconApp.hpp"
#include "yui/app/WifiProbeApp.hpp"
#include "yui/app/WifiHandshakeApp.hpp"
#include "yui/app/RemoteHeadApp.hpp"
#include "yui/app/AprsMessageApp.hpp"
#include "yui/app/HandshakeBrowserApp.hpp"
#include "yui/app/EvilTwinApp.hpp"
#include "yui/app/KarmaApp.hpp"
#include "yui/app/DeauthApp.hpp"
#include "yui/app/BleSpamApp.hpp"
#include "yui/app/WifiBeaconFloodApp.hpp"
#include "yui/app/WpsScanApp.hpp"
#include "yui/app/BleGattApp.hpp"
#include "yui/app/BleJammerApp.hpp"
#include "yui/app/CaptivePortalApp.hpp"
#include "yui/app/SatTrackerApp.hpp"
#include "yui/app/KoiGotchiApp.hpp"
#include "yui/app/ThemeApp.hpp"
#include "yui/app/SubGhzScanApp.hpp"
#include "yui/app/SubGhzCaptureApp.hpp"
#include "yui/app/SubGhzReplayApp.hpp"
#include "yui/app/SubGhzBruteApp.hpp"
#include "yui/app/SubGhzJammerApp.hpp"
#include "yui/app/Nrf24ScanApp.hpp"
#include "yui/app/Nrf24JammerApp.hpp"
#include "yui/app/MousejackApp.hpp"
#include "yui/app/RollJamApp.hpp"
#include "yui/app/HydraStatusApp.hpp"
#include "hal/esp32/Esp32Display.hpp"
#include "hal/esp32/Esp32Clock.hpp"
#include "hal/esp32/Esp32Log.hpp"
#include "hal/esp32/Esp32Keyboard.hpp"
#include "hal/esp32/Esp32Net.hpp"
#include "hal/esp32/Esp32Imu.hpp"
#include "hal/esp32/Esp32Fs.hpp"
#include "hal/esp32/Esp32Ir.hpp"
#include "hal/esp32/Esp32Mic.hpp"
#include "hal/esp32/Esp32Speaker.hpp"
#include "hal/esp32/Esp32Storage.hpp"
#include "hal/esp32/Esp32RadioLink.hpp"
#include "hal/esp32/Esp32Gnss.hpp"
#include "hal/esp32/Esp32Http.hpp"
#include "hal/esp32/Esp32Pcap.hpp"
#include "hal/esp32/Esp32WifiMonitor.hpp"
#include "hal/esp32/Esp32BleAdvertiser.hpp"
#include "hal/esp32/Esp32WifiAp.hpp"
#include "hal/esp32/Esp32BleCentral.hpp"
#include "hal/esp32/Esp32WebRemote.hpp"
#include "hal/esp32/Cc1101Radio.hpp"
#include "hal/esp32/Nrf24Radio.hpp"
#include "yui/app/RemoteApp.hpp"
#include <WiFi.h>
#include <esp_system.h>
#include <cstring>

namespace {
// Stamped by tools/inject_version.py at build time from `git describe`.
// Falls back to "dev" if the pre-build hook didn't run (e.g. someone built
// with a stripped-down env).
#ifndef YUI_VERSION
#define YUI_VERSION "dev"
#endif
constexpr const char* kVersion = YUI_VERSION;

yui::Esp32Display  display;
yui::Esp32Clock    clock_;
yui::SerialLog     log_;
yui::Esp32Keyboard keyboard;
yui::Hal           hal{display, keyboard, clock_, log_};

yui::Esp32Net     net_;
yui::Esp32Imu     imu_;
yui::Esp32Fs      fs_;
yui::Esp32Ir      ir_;
yui::Esp32Mic     mic_;
yui::Esp32Speaker spk_;
yui::Esp32Storage   store_;
yui::Esp32RadioLink radio_;   // BT-Classic SPP to TH-D75 (stub until v0.2)
yui::Esp32Gnss      gnss_;    // GPS feed via radio_ NMEA (stub until v0.2)
yui::Esp32Http      http_;    // HTTPClient wrapper for Pineapple REST
yui::Esp32Pcap      pcap_;    // libpcap writer to SD (stub until v0.2)
yui::Esp32WifiMonitor wmon_;  // promiscuous-mode RX (stub until v0.2)
yui::Esp32BleAdvertiser ble_adv_;  // BLE TX (stub until v0.3)
yui::Esp32WifiAp        wifi_ap_;  // SoftAP + captive portal (stub until v0.3)
yui::Esp32BleCentral    ble_cent_; // BLE central (stub until v0.3)
yui::Esp32WebRemote     remote_;   // Browser viewer + key inbox

// Pingequa Hydra RF Cap 424 — sub-GHz + 2.4 GHz radios. Both are
// nullable in spirit: probed at boot and left disabled if absent.
yui::Cc1101Radio cc1101_;
yui::Nrf24Radio  nrf24_;
bool cc1101_present_ = false;
bool nrf24_present_  = false;

// Sysinfo probes pull from M5/ESP/WiFi globals.
yui::SysProbe make_sys_probe() {
  yui::SysProbe p;
  p.battery_pct      = []() -> int { return M5.Power.getBatteryLevel(); };
  p.free_heap_bytes  = []() -> uint32_t { return ESP.getFreeHeap(); };
  p.uptime_ms        = []() -> uint32_t { return millis(); };
  p.ip_or_empty      = []() -> const char* {
    static String s;
    s = WiFi.isConnected() ? WiFi.localIP().toString() : String();
    return s.c_str();
  };
  p.wifi_rssi        = []() -> int { return WiFi.isConnected() ? WiFi.RSSI() : 0; };
  p.epoch_seconds    = []() -> uint64_t { return clock_.epoch_seconds(); };
  p.hydra_state      = []() -> int {
    const bool cc = cc1101_present_;
    const bool nr = nrf24_present_;
    if (cc && nr) return 1;
    if (cc || nr) return 2;
    return 3;
  };
  return p;
}

yui::AppRegistry registry;
yui::AboutApp    about_app{kVersion};
yui::RemoteApp   remote_app{remote_, &store_};
yui::WifiApp     wifi_app{net_, &store_};
yui::BleApp      ble_app{net_};
yui::CalculatorApp calc_app;
yui::FilesApp    files_app{fs_};
yui::IrRemoteApp ir_app{ir_};
yui::SettingsApp settings_app{store_};
yui::ClockApp    clock_app{&net_, "pool.ntp.org", "UTC0", &store_};
yui::SysinfoApp  sysinfo_app{make_sys_probe()};
yui::AprsApp     aprs_app{radio_};
yui::KenwoodApp  kenwood_app{radio_, &store_};
yui::GpsApp      gps_app{gnss_, fs_};
yui::PineappleApp        pa_app{http_, store_};
yui::PineappleReconApp   pa_recon_app{http_, store_};
yui::WifiProbeApp        probe_app{wmon_};
yui::WifiHandshakeApp    handshake_app{wmon_, pcap_, clock_};
yui::RemoteHeadApp       remote_head_app{radio_};
yui::AprsMessageApp      aprs_msg_app{radio_, store_};
yui::HandshakeBrowserApp pa_handshakes_app{http_, store_};
yui::EvilTwinApp         evil_twin_app{http_, store_};
yui::KarmaApp            karma_app{http_, store_};
yui::DeauthApp           deauth_app{http_, store_, wmon_};
yui::BleSpamApp          ble_spam_app{ble_adv_};
yui::WifiBeaconFloodApp  beacon_flood_app{wmon_};
yui::WpsScanApp          wps_scan_app{wmon_};
yui::BleGattApp          ble_gatt_app{ble_cent_};
yui::BleJammerApp        ble_jammer_app{ble_adv_};
yui::CaptivePortalApp    captive_app{wifi_ap_, fs_, store_};
yui::SatTrackerApp       sat_app{radio_, gnss_, fs_, clock_};
yui::KoiGotchiApp        koigotchi_app{handshake_app, &store_, &clock_};
yui::ThemeApp            theme_app{&store_};

// Phase 4 — Hydra RF apps. Each takes nullable radio pointers so the
// app can render a "cap not detected" dialog instead of crashing when
// the user enters it without the cap plugged in.
yui::SubGhzScanApp       subghz_scan_app{&cc1101_};
yui::SubGhzCaptureApp    subghz_capture_app{&cc1101_, &fs_, clock_};
yui::SubGhzReplayApp     subghz_replay_app{&cc1101_, &fs_};
yui::SubGhzBruteApp      subghz_brute_app{&cc1101_, clock_};
yui::SubGhzJammerApp     subghz_jammer_app{&cc1101_};
yui::Nrf24ScanApp        nrf24_scan_app{&nrf24_};
yui::Nrf24JammerApp      nrf24_jammer_app{&nrf24_};
yui::MousejackApp        mousejack_app{&nrf24_, clock_};
yui::RollJamApp          rolljam_app{&cc1101_, clock_};
yui::HydraStatusApp      hydra_status_app{&cc1101_, &nrf24_};

yui::Launcher* launcher_ptr = nullptr;
yui::Shell*    shell_ptr    = nullptr;
}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);

  // Bring up the offscreen sprite that backs every render call. Without this
  // we'd repaint a live framebuffer at 30 FPS and the panel would tear
  // diagonally on every frame.
  if (!display.begin()) {
    log_.error("display sprite alloc failed — falling back to direct draw");
  }

  // The Cardputer ADV's TCA8418 keypad lives on M5.In_I2C (G8/G9). M5Unified
  // only auto-begins that bus when it positively identifies the board; if
  // detection misses (the ADV is a relatively new SKU) the keyboard stays
  // dead. Bring the bus up explicitly so we don't depend on auto-detect.
  M5.In_I2C.begin(I2C_NUM_0, yui::pins::kIntSda, yui::pins::kIntScl);

  log_.info("Yui boot");

  // Probe the Hydra RF Cap 424 (CC1101 + nRF24L01+). Either chip may
  // be absent — apps that need them check the present flags and show
  // a "cap not detected" dialog rather than crashing.
  cc1101_present_ = cc1101_.begin();
  nrf24_present_  = nrf24_.begin();
  {
    char hbuf[64];
    std::snprintf(hbuf, sizeof(hbuf), "[hydra] cc1101=%s nrf24=%s",
                  cc1101_present_ ? "OK" : "absent",
                  nrf24_present_  ? "OK" : "absent");
    log_.info(hbuf);
  }

  // Boot-time WiFi auto-connect: if a saved SSID exists in NVS, kick off
  // the join + NTP sync now so ClockApp's TimeOfDay is ready by the time
  // the user opens it. Errors are silent; WifiApp surfaces re-join.
  store_.init();

  // Restore the user's chosen palette before any UI renders. Splash and
  // every app that follows reads ui::kAccent / ui::kSurface live, so the
  // first frame already wears the saved theme.
  {
    char theme_id[16] = {0};
    if (store_.get_str(yui::kStorageKeyTheme, theme_id, sizeof(theme_id))) {
      if (auto* p = yui::ui::find_palette(theme_id)) yui::ui::apply_palette(*p);
    }
  }

  char saved_ssid[33] = {0};
  char saved_pass[65] = {0};
  char saved_tz[40]   = "UTC0";
  char saved_ntp[40]  = "pool.ntp.org";
  store_.get_str("clock.tz",  saved_tz,  sizeof(saved_tz));
  store_.get_str("clock.ntp", saved_ntp, sizeof(saved_ntp));
  if (saved_tz[0]  == 0) std::strcpy(saved_tz,  "UTC0");
  if (saved_ntp[0] == 0) std::strcpy(saved_ntp, "pool.ntp.org");
  if (store_.get_str("wifi.ssid", saved_ssid, sizeof(saved_ssid)) &&
      saved_ssid[0] != 0) {
    store_.get_str("wifi.pass", saved_pass, sizeof(saved_pass));
    log_.info("WiFi auto-connect");
    net_.wifi_connect(saved_ssid, saved_pass);
    // ntp_sync is also called once association completes — but configTzTime
    // is safe to call before connect; SNTP retries internally once IP is up.
    net_.ntp_sync(saved_ntp, saved_tz);
  }

  // RADIO category (v0.2)
  registry.add(&kenwood_app);
  registry.add(&aprs_app);
  registry.add(&gps_app);
  registry.add(&remote_head_app);
  registry.add(&aprs_msg_app);
  registry.add(&sat_app);
  // WIFI category
  registry.add(&wifi_app);
  registry.add(&pa_app);
  registry.add(&pa_recon_app);
  registry.add(&probe_app);
  registry.add(&handshake_app);
  registry.add(&pa_handshakes_app);
  registry.add(&evil_twin_app);
  registry.add(&karma_app);
  registry.add(&deauth_app);
  registry.add(&beacon_flood_app);
  registry.add(&wps_scan_app);
  registry.add(&captive_app);
  // BLUETOOTH category
  registry.add(&ble_spam_app);
  registry.add(&ble_gatt_app);
  registry.add(&ble_jammer_app);
  registry.add(&ble_app);
  // TOOLS / SYSTEM
  registry.add(&ir_app);
  registry.add(&files_app);
  registry.add(&calc_app);
  registry.add(&clock_app);
  registry.add(&koigotchi_app);
  registry.add(&sysinfo_app);
  registry.add(&settings_app);
  registry.add(&theme_app);
  // Phase 4 — Hydra RF apps (Pingequa Hydra Cap 424).
  registry.add(&subghz_scan_app);
  registry.add(&subghz_capture_app);
  registry.add(&subghz_replay_app);
  registry.add(&subghz_brute_app);
  registry.add(&subghz_jammer_app);
  registry.add(&nrf24_scan_app);
  registry.add(&nrf24_jammer_app);
  registry.add(&mousejack_app);
  registry.add(&rolljam_app);
  registry.add(&hydra_status_app);
  registry.add(&remote_app);
  registry.add(&about_app);

  static yui::Launcher launcher{registry, make_sys_probe()};
  // Device splash holds for the full koi animation length; native env
  // keeps its 1.5 s default. Press a key after the floor elapses to skip.
  static yui::Shell    shell{hal, launcher, kVersion,
                             yui::kBootAnimationTotalMs};
  shell.set_splash_renderer(
      [](yui::IDisplay& d, uint32_t elapsed) {
        yui::render_boot_animation(d, elapsed);
      });
  launcher_ptr = &launcher;
  shell_ptr    = &shell;

  // Wire up the browser viewer. The display owns the sprite that the
  // remote streams; the keyboard hands its inbox back to the same remote.
  remote_.set_canvas(display.canvas());
  remote_.set_version(kVersion);
  keyboard.set_remote(&remote_);
  shell.bind_remote(&remote_, &store_);

  // Persisted dev-mode flag: if set, fire up the viewer once we've got an
  // IP. (start() is a no-op while WiFi is still associating; we retry from
  // loop().)
  int32_t dev_remote = 0;
  store_.get_int(yui::kStorageKeyDevRemote, dev_remote, 0);
  if (dev_remote) remote_.start();

  shell.start();
}

void loop() {
  shell_ptr->tick();
  // Service the HTTP + WS server, push a frame if it's time. Both are
  // throttled internally so this stays cheap when nobody's connected.
  remote_.tick();
  remote_.push_frame();

  // Retry remote start once WiFi finishes associating after boot.
  static bool boot_remote_started = false;
  if (!boot_remote_started && !remote_.running()) {
    int32_t dev_remote = 0;
    store_.get_int(yui::kStorageKeyDevRemote, dev_remote, 0);
    if (dev_remote && WiFi.isConnected()) {
      remote_.start();
      boot_remote_started = true;
    }
  }
  delay(33);  // ~30 FPS
}

#endif  // YUI_TARGET_CARDPUTER_ADV

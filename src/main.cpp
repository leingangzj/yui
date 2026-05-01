// Yui — M5Cardputer ADV entry point.
// Real hardware build only. Native env excludes this file (see platformio.ini).

#if defined(YUI_TARGET_CARDPUTER_ADV)

#include <M5Unified.h>
#include "yui/shell/Splash.hpp"
#include "yui/shell/Shell.hpp"
#include "yui/app/AppRegistry.hpp"
#include "yui/app/Launcher.hpp"
#include "yui/app/AboutApp.hpp"
#include "yui/app/StubApp.hpp"
#include "yui/app/WifiApp.hpp"
#include "yui/app/BleApp.hpp"
#include "yui/app/CalculatorApp.hpp"
#include "yui/app/ImuApp.hpp"
#include "yui/app/NotesApp.hpp"
#include "yui/app/FilesApp.hpp"
#include "yui/app/IrRemoteApp.hpp"
#include "yui/app/MicApp.hpp"
#include "yui/app/SettingsApp.hpp"
#include "yui/app/ClockApp.hpp"
#include "yui/app/SysinfoApp.hpp"
#include "yui/app/SnakeApp.hpp"
#include "yui/app/KeyTestApp.hpp"
#include "yui/app/ToneApp.hpp"
#include "yui/app/PomodoroApp.hpp"
#include "yui/app/MetronomeApp.hpp"
#include "yui/app/LifeApp.hpp"
#include "yui/app/DrawApp.hpp"
#include "yui/app/CalendarApp.hpp"
#include "yui/app/TodoApp.hpp"
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
#include <WiFi.h>
#include <esp_system.h>

namespace {
constexpr const char* kVersion = "v0.0.1-dev";

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
yui::Esp32Storage store_;

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
  return p;
}

yui::AppRegistry registry;
yui::AboutApp    about_app{kVersion};
yui::WifiApp     wifi_app{net_};
yui::BleApp      ble_app{net_};
yui::ImuApp      imu_app{imu_};
yui::CalculatorApp calc_app;
yui::NotesApp    notes_app{fs_};
yui::FilesApp    files_app{fs_};
yui::IrRemoteApp ir_app{ir_};
yui::MicApp      mic_app{mic_};
yui::SettingsApp settings_app{store_};
yui::ClockApp    clock_app{&net_};
yui::SysinfoApp  sysinfo_app{make_sys_probe()};
yui::SnakeApp    snake_app;
yui::KeyTestApp  keytest_app;
yui::ToneApp     tone_app{spk_};
yui::PomodoroApp pomodoro_app{spk_};
yui::MetronomeApp metronome_app{spk_};
yui::LifeApp     life_app;
yui::DrawApp     draw_app{fs_};
yui::CalendarApp calendar_app;
yui::TodoApp     todo_app{fs_};

yui::Launcher* launcher_ptr = nullptr;
yui::Shell*    shell_ptr    = nullptr;
}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);

  log_.info("Yui boot");

  registry.add(&wifi_app);
  registry.add(&ble_app);
  registry.add(&ir_app);
  registry.add(&imu_app);
  registry.add(&notes_app);
  registry.add(&todo_app);
  registry.add(&files_app);
  registry.add(&calc_app);
  registry.add(&mic_app);
  registry.add(&tone_app);
  registry.add(&metronome_app);
  registry.add(&pomodoro_app);
  registry.add(&clock_app);
  registry.add(&snake_app);
  registry.add(&life_app);
  registry.add(&draw_app);
  registry.add(&calendar_app);
  registry.add(&keytest_app);
  registry.add(&sysinfo_app);
  registry.add(&settings_app);
  registry.add(&about_app);

  static yui::Launcher launcher{registry, make_sys_probe()};
  static yui::Shell    shell{hal, launcher, kVersion};
  launcher_ptr = &launcher;
  shell_ptr    = &shell;
  shell.start();
}

void loop() {
  shell_ptr->tick();
  delay(33);  // ~30 FPS
}

#endif  // YUI_TARGET_CARDPUTER_ADV

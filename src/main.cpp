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
#include "hal/esp32/Esp32Display.hpp"
#include "hal/esp32/Esp32Clock.hpp"
#include "hal/esp32/Esp32Log.hpp"
#include "hal/esp32/Esp32Keyboard.hpp"
#include "hal/esp32/Esp32Net.hpp"
#include "hal/esp32/Esp32Imu.hpp"
#include "hal/esp32/Esp32Fs.hpp"
#include "hal/esp32/Esp32Ir.hpp"
#include "hal/esp32/Esp32Mic.hpp"

namespace {
constexpr const char* kVersion = "v0.0.1-dev";

yui::Esp32Display  display;
yui::Esp32Clock    clock_;
yui::SerialLog     log_;
yui::Esp32Keyboard keyboard;
yui::Hal           hal{display, keyboard, clock_, log_};

yui::Esp32Net    net_;
yui::Esp32Imu    imu_;
yui::Esp32Fs     fs_;
yui::Esp32Ir     ir_;
yui::Esp32Mic    mic_;

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
  registry.add(&files_app);
  registry.add(&calc_app);
  registry.add(&mic_app);
  registry.add(&about_app);

  static yui::Launcher launcher{registry};
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

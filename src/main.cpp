// Yui — M5Cardputer ADV entry point.
// Real hardware build only. Native env excludes this file (see platformio.ini).

#if defined(YUI_TARGET_CARDPUTER_ADV)

#include <M5Unified.h>
#include "splash.hpp"
#include "hal/esp32/Esp32Display.hpp"
#include "hal/esp32/Esp32Clock.hpp"
#include "hal/esp32/Esp32Log.hpp"
#include "hal/esp32/Esp32Keyboard.hpp"

namespace {
constexpr const char* kVersion = "v0.0.1-dev";

yui::Esp32Display display;
yui::Esp32Clock   clock;
yui::SerialLog    log;
yui::Esp32Keyboard keyboard;

uint32_t splash_start_ms = 0;
}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);

  log.info("Yui boot");
  splash_start_ms = clock.millis();
}

void loop() {
  // v0.0: splash animates forever. v0.1 will hand off to the launcher
  // after a press / timeout.
  uint32_t elapsed = clock.millis() - splash_start_ms;
  yui::render_splash(display, kVersion, elapsed);
  delay(33);  // ~30 FPS
}

#endif  // YUI_TARGET_CARDPUTER_ADV

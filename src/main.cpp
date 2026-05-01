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
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);

  yui::Esp32Display display;
  yui::Esp32Clock   clock;
  yui::SerialLog    log;
  yui::Esp32Keyboard keyboard;

  log.info("Yui boot");
  yui::render_splash(display, kVersion);
}

void loop() {
  // TODO: shell event loop — for v0.0 we just sit on the splash
  delay(50);
}

#endif  // YUI_TARGET_CARDPUTER_ADV

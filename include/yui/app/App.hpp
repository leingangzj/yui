#pragma once
#include "yui/hal/Hal.hpp"
#include "yui/hal/IKeyboard.hpp"
#include "yui/assets/icons.hpp"

namespace yui {

// Top-level launcher buckets. Flipper-Zero-style: home screen shows
// these categories; user drills into one to get an app list.
enum class Category : uint8_t {
  Radio,    // TH-D75 / APRS / GPS / SatTracker / audio decode (v0.2/v0.3)
  WiFi,     // Scan / Handshake / Probes / Pineapple
  Bluetooth,
  Tools,    // Field utilities: Calc, Notes, Files, IR, Mic
  System,   // Clock, Sysinfo, Settings, About, KeyTest
  Fun,      // Toys: Snake, Life, Draw, Tone, Metronome, Pomodoro, Calendar, Todo, IMU
};

inline const char* category_label(Category c) {
  switch (c) {
    case Category::Radio:     return "Radio";
    case Category::WiFi:      return "WiFi";
    case Category::Bluetooth: return "Bluetooth";
    case Category::Tools:     return "Tools";
    case Category::System:    return "System";
    case Category::Fun:       return "Fun";
  }
  return "?";
}

// Base for every Yui app. Apps don't draw or pump events themselves — the
// Shell tells them when to render, ticks them at a steady cadence, and feeds
// them key events. All HAL access flows through the Hal& given on entry.
class App {
public:
  virtual ~App() = default;

  virtual const char* name() const = 0;

  // Default bucket is Tools — apps that aren't a clear fit live here.
  virtual Category category() const { return Category::Tools; }

  // Optional 24x24 PNG shown next to the app name in the launcher's
  // app-list view. Default returns nullptr so the launcher falls back
  // to the category icon. Override to give an app its own glyph.
  virtual const assets::IconRef* icon() const { return nullptr; }

  virtual void on_enter(Hal& /*hal*/) {}
  virtual void on_key(KeyEvent /*k*/) {}
  virtual void tick(uint32_t /*now_ms*/) {}
  virtual void render(IDisplay& d) = 0;
  virtual void on_exit() {}
};

}  // namespace yui

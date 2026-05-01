# Architecture

## Guiding Principles

1. **HAL first.** Every byte of hardware access goes through an interface in
   `include/yui/hal/`. No app code calls `Wire.beginTransmission()` directly.
   This is what lets us run the same firmware on the device _and_ in a desktop
   test harness.
2. **Apps are small and finishable.** Each app implements a tiny interface
   (`init`, `tick`, `render`, `on_key`, `cleanup`). One file when possible.
3. **No dynamic allocation in the hot path.** Heap use at startup is fine;
   per-frame allocations are a bug.
4. **Keep dependencies thin.** The only mandatory libs are M5GFX (display) and
   the ESP32 Arduino core. Per-app libs (BLE, IR, etc.) are pulled in only
   where used and gated behind build flags.
5. **MIT-clean.** No code copied from AGPL/GPL projects (Bruce, NEMO). We may
   read them; we may not paste from them.

## Layers

```
┌─────────────────────────────────────────────┐
│  Apps        notes, wifi_scan, ir_remote,…  │
├─────────────────────────────────────────────┤
│  Shell       launcher, menu, theme, nav     │
├─────────────────────────────────────────────┤
│  Services    storage, time, settings, evt   │
├─────────────────────────────────────────────┤
│  HAL         IDisplay, IKeyboard, IClock,…  │  ← only this is mocked
├─────────────────────────────────────────────┤
│  Backends    esp32/  |  native/             │
└─────────────────────────────────────────────┘
```

The same Shell + Apps build against either backend. Native backend is for
host-side tests (and later, an SDL desktop simulator).

## HAL Interfaces (initial set)

| Interface   | Purpose                                                    |
| ----------- | ---------------------------------------------------------- |
| `IDisplay`  | Pixel-level drawing, text, fills, push-buffer              |
| `IKeyboard` | Key events (down/up/repeat) + modifier state               |
| `IClock`    | Monotonic ms + wall-clock time                             |
| `IStorage`  | Read/write key-value blobs (NVS on device, file on native) |
| `IFs`       | microSD / filesystem ops                                   |
| `INet`      | WiFi scan/connect, BLE scan                                |
| `IRadio`    | IR transmit, IMU read                                      |
| `ILog`      | `info` / `warn` / `error` sinks                            |

Interfaces live in `include/yui/hal/*.hpp`. Implementations:

- `src/hal/esp32/*.cpp` — real hardware (Arduino core / M5GFX / ESP-IDF)
- `src/hal/native/*.cpp` — no-ops, fakes, in-memory state for host tests

## App Interface

```cpp
class App {
public:
  virtual ~App() = default;
  virtual const char* name() const = 0;
  virtual void on_enter(Hal& hal) = 0;     // app becomes active
  virtual void on_key(KeyEvent k) = 0;     // user input
  virtual void tick(uint32_t now_ms) = 0;  // cooperative scheduling
  virtual void render(IDisplay& d) = 0;    // draw frame
  virtual void on_exit() = 0;              // free resources
};
```

Apps register themselves at startup via a static registry (no dynamic
discovery). Adding an app = one new file + one line in `src/app_registry.cpp`.

## Build Profiles

Two PlatformIO environments to start:

- `cardputer_adv` — real device build, all hardware backends
- `native` — host x86 build for unit tests, native HAL backend, no Arduino

We may add `cardputer_base` (original Cardputer) and `desktop_sdl` later. We
don't pre-build for hardware we don't own.

## Memory Targets (soft)

- **Flash:** stay under 4 MB for the firmware partition (8 MB total, leaves
  room for a SPIFFS/LittleFS data partition + OTA later).
- **RAM:** apps shouldn't allocate more than ~32 KB of long-lived state.

## What Yui Is Not

- Not a Flipper Zero clone. We don't have the radios.
- Not a security-research toolkit. WiFi/BLE features are convenience-grade
  (scan, connect, info), not deauth/portal/sniff.
- Not Meshtastic. If you want mesh, install Meshtastic — it's pre-flashed by
  M5Stack with the Mesh Kit. Yui can coexist with a separate flash slot later.

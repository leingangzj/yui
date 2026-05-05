# Architecture

## Principles

1. **HAL first.** Every byte of hardware access goes through an interface in `include/yui/hal/`. App code doesn't `#include <Wire.h>` or call `digitalWrite()` directly. This is what lets the same firmware build for the device and run in a host test harness.
2. **Apps are small and finishable.** Each app implements a tiny interface (`on_enter` / `on_key` / `tick` / `render` / `on_exit`). One file when possible.
3. **No allocation in the hot path.** Heap at startup is fine; per-frame `new` / `std::string` is a bug.
4. **Thin dependencies.** Mandatory: M5GFX, M5Unified, Arduino-ESP32. Per-feature: NimBLE, RadioLib, IRremoteESP8266, WebSockets — gated behind app inclusion.
5. **MIT-clean.** No code copied from AGPL/GPL projects (Bruce, Marauder, NEMO). Reading them is fine; pasting from them is not.

## Layers

```
┌─────────────────────────────────────────────┐
│  Apps        ~32 launcher entries           │
├─────────────────────────────────────────────┤
│  Shell       launcher, menu, theme, nav     │
├─────────────────────────────────────────────┤
│  Services    storage, time, settings, evt   │
├─────────────────────────────────────────────┤
│  HAL         IDisplay, IKeyboard, IClock... │  ← only this is mocked
├─────────────────────────────────────────────┤
│  Backends    esp32/  |  native/             │
└─────────────────────────────────────────────┘
```

Same Shell + Apps build against either backend. The native backend runs the host-side test suite (387 cases, ~4 s) and could later drive an SDL desktop simulator.

## HAL interfaces

| Interface        | Purpose                                                  |
| ---------------- | -------------------------------------------------------- |
| `IDisplay`       | Pixel drawing, styled text, fills, push-buffer           |
| `IKeyboard`      | Key events (down/up) + modifier state                    |
| `IClock`         | Monotonic ms + wall-clock + epoch seconds                |
| `IStorage`       | NVS key/value (file-backed on native)                    |
| `IFs`            | microSD / filesystem ops                                 |
| `INet`           | WiFi scan/connect, BLE scan, NTP                         |
| `ILog`           | `info` / `warn` / `error` sinks                          |
| `IIr`            | IR TX / decode                                           |
| `IImu`           | Accel + gyro reads                                       |
| `IMic`           | PCM sample stream                                        |
| `ISpeaker`       | Tone / waveform out                                      |
| `IPcap`          | libpcap-format writer to SD                              |
| `IWifiMonitor`   | Promiscuous-mode RX + raw 802.11 TX                      |
| `IBleAdvertiser` | NimBLE GAP advertise                                     |
| `IBleCentral`    | NimBLE BLEClient + GATT walk                             |
| `IWifiAp`        | SoftAP + DNS server + HTTP captive portal                |
| `IRadioLink`     | BT-Classic SPP for the TH-D75 (via bb-link bridge)       |
| `IGnss`          | NMEA stream from the radio                               |
| `IHttp`          | HTTPClient wrapper for Pineapple REST                    |
| `IBleRawTx`      | Per-channel raw BLE TX (NimBLE + nRF24 dual-rail)        |
| `ICc1101`        | Sub-GHz radio: tune, OOK/FSK, edge-toggle TX, packet IRQ |
| `INrf24`         | 2.4 GHz raw radio: channel, TX/RX, RPD-bit promiscuous   |

Implementations:

- `src/hal/esp32/*.hpp` — real hardware via Arduino-ESP32 / M5Unified / NimBLE / ESP-IDF / RadioLib
- `src/hal/native/*.hpp` — fakes with in-memory state and test seams

## App interface

```cpp
class App {
public:
  virtual ~App() = default;
  virtual const char* name() const = 0;
  virtual Category    category() const = 0;
  virtual void on_enter(Hal& hal) = 0;     // app becomes active
  virtual void on_key(KeyEvent k) = 0;     // user input
  virtual void tick(uint32_t now_ms) {}    // cooperative scheduling
  virtual void render(IDisplay& d) = 0;    // draw a frame
  virtual void on_exit() {}                // free resources
};
```

Apps register at startup via `AppRegistry::add(&app_instance)` from `src/main.cpp`. No dynamic discovery; `kMaxApps = 64`. Adding an app = one new header + one line in `main.cpp`.

## Build profiles

| Env             | Purpose                                                       |
| --------------- | ------------------------------------------------------------- |
| `cardputer_adv` | Real device build, all ESP32 backends                         |
| `native`        | Host x86 build for unit tests, native HAL backend, no Arduino |

`cardputer_base` (original Cardputer) and `desktop_sdl` are possibilities; not pre-built for hardware nobody owns.

## Memory targets

- **Flash:** ~2 MB / 8 MB (huge_app partition leaves room for SPIFFS / OTA later)
- **RAM:** apps under ~32 KB long-lived; current footprint ~56% of 320 KB

## Lab-mode override path

The deauth-block override (`ieee80211_freedom_inside_cb` → `return 1`) lives as committed C source at `src/lib_extra_override/freedom_override.c` rather than a vendored binary. With `-Wl,-zmuldefs` in the link, the project archive's symbol wins over Espressif's `libnet80211.a`. See [`board/lib_extra/README.md`](../board/lib_extra/README.md) for the audit trail and the alternative pre-built path.

## What Yui is not

- **Not a Flipper Zero.** It can do most of the same RF tricks with the Hydra cap, but no NFC, no 125 kHz, no IButton.
- **Not a Pineapple replacement.** The Pineapple suite drives a real Pineapple over its REST API; it doesn't reimplement PineAP.
- **Not Meshtastic.** If you want mesh, flash Meshtastic — M5Stack ship a Mesh Kit pre-flashed for it. Yui can coexist in a separate slot when OTA lands.

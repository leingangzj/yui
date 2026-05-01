# Yui

> 結い — _Okinawan: cooperative community work, the bond that ties people together._

A community-driven hobby OS for the **M5Stack Cardputer ADV**.

Yui is a tiny pocket-sized utility shell — a launcher plus a growing suite of
small apps (notes, files, WiFi/BLE tools, IR remote, IMU toys, calculator,
microSD browser). It runs on the bare ADV with no extra hardware required, and
can grow as you add Caps and Grove modules.

## Status

🌱 **v0.0 — ideation + scaffold.** No flashable build yet.

## Goals

- **Pocket-sized.** One device, one keyboard, one screen. Apps stay small and
  finishable.
- **Hobbyist-first.** Friendly, hackable, MIT-licensed. Drop-in your own app in
  an afternoon.
- **Hardware-honest.** v0.1 only uses what the ADV has onboard (WiFi, BLE, IR,
  IMU, mic, speaker, microSD, screen, keyboard). External modules are future
  milestones, not promises.
- **Testable on a laptop.** HAL-first architecture means most code runs and is
  unit-tested on a host machine; we only need real hardware for final validation.

## Hardware Target

**M5Stack Cardputer ADV** — ESP32-S3FN8, 8MB flash, 1.14" ST7789V2 LCD
(240×135), 56-key keyboard via TCA8418, BMI270 IMU, ES8311 audio + 3.5mm jack,
MEMS mic, IR emitter, microSD, Grove HY2.0-4P, EXT 14-pin bus, 1750 mAh
battery.

See [docs/HARDWARE.md](docs/HARDWARE.md) for verified specs and pinout.

## Quick Look

```
docs/             ARCHITECTURE, HARDWARE, TEST_ENV, PRIOR_ART, ROADMAP
include/yui/      public headers
include/yui/hal/  hardware abstraction interfaces
src/              implementations (esp32 + native backends)
test/test_native/ host-side unit tests (PlatformIO `test_native`)
platformio.ini    cardputer + native env definitions
```

## Building (planned, not yet wired)

```bash
# Run tests on this machine (no hardware needed)
pio test -e native

# Build for the ADV
pio run -e cardputer_adv

# Flash + monitor
pio run -e cardputer_adv -t upload -t monitor
```

## License

MIT — see [LICENSE](LICENSE). Keep the copyright notice and do whatever you
want with it.

## Roadmap

See [docs/ROADMAP.md](docs/ROADMAP.md). Short version: launcher + ~8 onboard-only
apps for v0.1, then external-module support after.

## Prior Art

Yui borrows ideas (not code) from [Bruce](https://github.com/BruceDevices/firmware)
and [NEMO](https://github.com/n0xa/m5stick-nemo). Both are excellent; both are
GPL/AGPL. Yui is MIT and written fresh. See [docs/PRIOR_ART.md](docs/PRIOR_ART.md).

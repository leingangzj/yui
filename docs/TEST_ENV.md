# Test Environment

Two-layer test setup so most development happens without flashing the device.

## Layer 1 — Native unit tests (PlatformIO `native` env)

Anything that doesn't directly poke registers — menu state, app dispatch, settings serialization, .sub / EAPOL / NDEF / dot11 parsers, key-event handling, theme rendering against a fake display, app interaction tests against fake HALs.

```bash
pio test -e native     # ~4 seconds, 387 cases
```

PlatformIO's `native` platform compiles for the host (x86_64 Linux/macOS). Tests use [Unity](https://github.com/ThrowTheSwitch/Unity) (the C test framework, bundled with PIO).

Backends used during native tests:

| HAL              | Native impl                                                                                                          |
| ---------------- | -------------------------------------------------------------------------------------------------------------------- |
| `IDisplay`       | `NativeDisplay` — in-memory framebuffer; tests assert "pixel at (x,y) is color C" or "rendered text contains 'WiFi'" |
| `IKeyboard`      | `MockKeyboard` — tests inject key events directly                                                                    |
| `IClock`         | `FakeClock` — tests advance time deterministically                                                                   |
| `IStorage`       | `FakeStorage` — backed by `std::unordered_map`                                                                       |
| `IFs`            | `FakeFs` — in-memory file map                                                                                        |
| `INet`           | `FakeNet` — canned scan results, controllable BLE results                                                            |
| `IHttp`          | `FakeHttp` — registered URL → response table                                                                         |
| `ICc1101`        | `NativeCc1101` — present-flag toggle, RX queue, last-TX inspect, simulate-IRQ seam                                   |
| `INrf24`         | `NativeNrf24` — channel/address validation, listen state, force-fail switches                                        |
| `IBleAdvertiser` | `FakeBleAdvertiser` — set/enable counters, last-payload inspect                                                      |
| `IBleCentral`    | `FakeBleCentral` — register service tree, mac → service map                                                          |
| `IBleRawTx`      | `FakeNimBleRawTx` / `FakeNrf24BleRawTx` — channel mapping, tx counter                                                |
| `IWifiMonitor`   | `FakeWifiMonitor` — frame inject + tx_log capture                                                                    |
| `IWifiAp`        | `FakeWifiAp` — simulate captive-portal form submits                                                                  |
| `IPcap`          | `FakePcap` — bytes_written counter, in-memory capture                                                                |

Every PR runs `pio test -e native` in CI. Tests pass in under 5 seconds.

## Layer 2 — Wokwi simulator (browser / VS Code)

The full firmware, including LCD, keypad, and timing. [Wokwi](https://wokwi.com/esp32) supports ESP32-S3 + ST7789 240×135. There's no prebuilt Cardputer-ADV board, so:

1. **Bench composition** — ESP32-S3-DevKit + ST7789 + matrix keypad in Wokwi's diagram editor. Lower fidelity but zero extra work.
2. **Custom chip** — author a `cardputer-adv.chip.json` + driver that mimics the TCA8418 + display in one component. About half a day of work; then it's a first-class clickable Cardputer in the browser.

Start with (1) once the firmware boots in Wokwi; (2) is a stretch goal.

## Layer 3 (later) — Desktop SDL build

A third PIO env (`desktop_sdl`) linking against SDL2 and rendering the firmware to a window with the PC keyboard mapped onto Cardputer keys. Same Shell + Apps, just a third HAL backend. Highest fidelity for UI work. Deferred until we feel the pain of (1) and (2) not being enough.

## What we are not doing

- **QEMU.** Espressif's fork runs ESP32-S3 but with poor peripheral support. LCD and keyboard would have to be added by us, and Wokwi already did that work better.
- **Renode.** Overkill for a single-board hobby project.
- **Hardware-in-the-loop CI.** No reason to set up a board farm for one device.

## Test conventions

- Tests live in `test/test_native/test_yui.cpp` (currently one file).
- One assertion per behavior; descriptive test names (`menu_wraps_around_at_end_of_list`).
- No tests for "the framework works" — only for our logic.
- Coverage is a side effect, not a goal. Chase confidence, not %.

## On-device probes

Two complementary on-device test paths:

| App                    | Purpose                                                                                                                        |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| `SelfTestApp`          | One-keypress 14-step bring-up; pass/skip/fail per peripheral. Replaces the paper checklist for routine bring-ups.              |
| `BbLinkProbeApp`       | Three-phase chain validation for the islandmagic/bb-link bridge.                                                               |
| `HydraStatusApp`       | Cap presence + per-radio telemetry. Diagnostic readout for the Pingequa Hydra cap.                                             |
| `SoakLog` (autonomous) | Appends one CSV row per minute to `/soak/<boot_ts>.csv`. Pair with `tools/soak-monitor.py --read-soak-csv` for trend analysis. |

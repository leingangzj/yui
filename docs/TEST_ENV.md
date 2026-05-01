# Test Environment

Yui uses a **two-layer test setup** so most development happens without
flashing the device.

## Layer 1 — Native unit tests (PlatformIO `test_native`)

**What runs here:** any code that doesn't directly poke registers — menu state
machine, app dispatch, settings serialization, NDEF/CSV parsers, key-event
debouncing, theme rendering against a fake display.

**How:** PlatformIO has a `native` platform that compiles for the host (x86_64
Linux/macOS). Tests use [Unity](https://github.com/ThrowTheSwitch/Unity) (the C
test framework, bundled with PIO).

```bash
pio test -e native
```

Backends used during native tests:

- `IDisplay` → `NativeDisplay` — writes to an in-memory framebuffer; tests can
  assert "pixel at (x,y) is color C" or "rendered text contains 'WiFi'".
- `IKeyboard` → `MockKeyboard` — tests inject key events directly.
- `IClock` → `FakeClock` — tests advance time deterministically.
- `IStorage` → `MemStorage` — backed by `std::unordered_map`.
- `INet` → `FakeNet` — returns canned scan results.

**Goal:** every PR runs `pio test -e native` in CI; tests pass in <5 seconds.

## Layer 2 — Wokwi simulator (browser / VS Code)

**What runs here:** the full firmware, including LCD, keypad, and timing.

[Wokwi](https://wokwi.com/esp32) supports ESP32-S3 + ST7789 240×135. There is
no prebuilt Cardputer-ADV board, so we'll compose one of two ways:

1. **Bench composition** — ESP32-S3-DevKit + ST7789 + matrix keypad in
   Wokwi's diagram editor. Lower fidelity but zero extra work.
2. **Custom chip** — author a `cardputer-adv.chip.json` + driver that mimics
   the TCA8418 + display in one component. ~half-day of work, then it's a
   first-class clickable Cardputer in the browser.

We'll start with (1) once the firmware boots; (2) is a stretch goal.

## Layer 3 (optional, later) — Desktop SDL build

A third PIO env (`desktop_sdl`) that links against SDL2 and renders the
firmware to a window on your laptop, with the PC keyboard mapped onto
Cardputer keys. Same Shell + Apps, just a third HAL backend. Highest fidelity
for UI work.

Deferred until we feel the pain of (1) and (2) not being enough.

## What we are _not_ doing

- **QEMU.** Espressif's QEMU fork runs ESP32-S3 but with poor peripheral
  support; LCD and keyboard would have to be added by us, and Wokwi already
  did that work better. Skip.
- **Renode.** Overkill for a single-board hobby project.
- **Hardware-in-the-loop CI.** No reason to set up a board farm for one device.

## Test Conventions

- Tests live in `test/test_native/` — one file per area
  (`test_menu.cpp`, `test_settings.cpp`, etc.).
- One assertion per behavior; descriptive test names
  (`menu_wraps_around_at_end_of_list`).
- No tests for "the framework works" — only for _our_ logic.
- Coverage is a side effect, not a goal. We chase confidence, not %.

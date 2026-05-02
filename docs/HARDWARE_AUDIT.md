# Hardware Audit — M5Cardputer ADV

**Date:** 2026-05-01
**Sources:**

- `docs.m5stack.com/en/core/Cardputer ADV` (official M5 docs)
- `M5Unified` library — `src/M5Unified.cpp` (board pin tables) and `src/utility/Power_Class.cpp`
- `M5GFX` library — `src/M5GFX.cpp` (LCD autodetect + pin assignments)

**Method:** cross-checked every line in `include/yui/config/pins.hpp` and `docs/HARDWARE.md` against M5Unified/M5GFX source (authoritative — our build pulls them in) and the official M5 product page.

---

## Result: 1 missing pin, 0 wrong pins, 1 documentation correction

### ✅ Verified correct (no change needed)

| Signal           | Yui pins.hpp     | M5 official | Status |
| ---------------- | ---------------- | ----------- | ------ |
| Internal I²C SDA | 8                | G8          | ✓      |
| Internal I²C SCL | 9                | G9          | ✓      |
| LCD MOSI         | 35               | G35 (DAT)   | ✓      |
| LCD SCK          | 36               | G36         | ✓      |
| LCD CS           | 37               | G37         | ✓      |
| LCD DC           | 34               | G34 (RS)    | ✓      |
| LCD RST          | 33               | G33         | ✓      |
| LCD BL           | 38               | G38         | ✓      |
| SD MOSI          | 14               | G14         | ✓      |
| SD MISO          | 39               | G39         | ✓      |
| SD CLK           | 40               | G40         | ✓      |
| SD CS            | 12               | G12         | ✓      |
| IR TX            | 44               | G44         | ✓      |
| I²S SCLK         | 41               | G41         | ✓      |
| I²S LRCK         | 43               | G43         | ✓      |
| I²S DSDIN (out)  | 42               | G42         | ✓      |
| I²S ASDOUT (mic) | 46               | G46         | ✓      |
| Battery ADC      | 10               | G10         | ✓      |
| Grove SDA        | 2                | G2          | ✓      |
| Grove SCL        | 1                | G1          | ✓      |
| EXT-14 RESET     | 3                | G3          | ✓      |
| EXT-14 INT       | 4                | G4          | ✓      |
| EXT-14 BUSY      | 6                | G6          | ✓      |
| EXT-14 CS        | 5                | G5          | ✓      |
| EXT-14 SCK       | 40 (shared w/SD) | G40         | ✓      |
| EXT-14 MOSI      | 14 (shared w/SD) | G14         | ✓      |
| EXT-14 MISO      | 39 (shared w/SD) | G39         | ✓      |
| EXT-14 I²C SDA   | 8 (shared)       | G8          | ✓      |
| EXT-14 I²C SCL   | 9 (shared)       | G9          | ✓      |
| EXT-14 UART RX   | 13               | G13         | ✓      |
| EXT-14 UART TX   | 15               | G15         | ✓      |

### ❌ Missing from pins.hpp

| Signal               | GPIO | Source              | Action                             |
| -------------------- | ---- | ------------------- | ---------------------------------- |
| TCA8418 keyboard INT | 11   | M5 official ADV doc | **Add `kKbdInt = 11`** to pins.hpp |

**Why it matters:** the TCA8418 has a key-event FIFO with an INT line that asserts when there are pending events. Without it, our driver polls I²C every tick (~30 ms) regardless of activity — burns ~3% CPU and a chunk of the I²C bus on a quiet keyboard. With INT wired, we only read the FIFO on edge.

For v0.1 this is fine (we polled and it worked). For v0.2 + v0.3, when we'll be running BLE serial, WiFi monitor, and audio I/O concurrently, every freed I²C cycle helps.

### 📝 Documentation correction

`docs/HARDWARE.md` lists the EXT-14 pin order incorrectly. The actual layout per the official M5 doc is:

| Pin   | Left     | Right        |
| ----- | -------- | ------------ |
| 1-2   | G3 RESET | 5VIN         |
| 3-4   | G4 INT   | GND          |
| 5-6   | G6 BUSY  | 5VOUT        |
| 7-8   | G40 SCK  | G8 (I²C SDA) |
| 9-10  | G14 MOSI | G9 (I²C SCL) |
| 11-12 | G39 MISO | G13 UART_RX  |
| 13-14 | G5 CS    | G15 UART_TX  |

Our HARDWARE.md has the **correct GPIOs** but mislabels Pin 8 as `I²C_SDA = GPIO 8` — the GPIO is right, but Pin 8 of the connector is on the right column, not the left. Cosmetic only; no code impact.

---

## Power management

`Power_Class.cpp:258` shows the ADV uses **no fuel-gauge IC** — just a raw ADC voltage divider on GPIO 10:

- `_pmic = pmic_t::pmic_adc`
- `_adc_ratio = 2.0f` (so `Vbat = ADC_voltage × 2`)

**Implication:** `M5.Power.getBatteryLevel()` returns a lookup-table approximation from voltage, not a coulomb-counted SoC. This is fine for the launcher header `BATT %` indicator but **not** accurate enough to show "X minutes remaining" predictions during long captures. Don't promise that.

When on USB power the divider sees ~5 V, which the lookup clamps to "100%" — so `battery_pct()` won't tell us if the user is plugged in. If we need that signal, we need to read the charger IC over I²C separately (TBD).

---

## SPI bus topology — important for v0.2/v0.3

The microSD card and the EXT-14 SPI Cap (e.g. LoRa-1262) share **the same SPI3 bus**:

- Common: SCK=40, MOSI=14, MISO=39
- microSD CS: 12
- EXT-14 CS: 5

**Implication:** simultaneous SD writes (e.g. `.pcap` capture) and Cap activity will need bus-arbitration discipline. M5Unified handles this via the SPI driver but we need to ensure our HAL doesn't lock out the Cap during long SD writes. Worth a v0.2 stress test.

---

## I²C bus topology — important when Caps arrive

The internal I²C bus (G8/G9) is shared between:

- TCA8418 keypad controller (`0x34`)
- BMI270 IMU (`0x68`)
- ES8311 audio codec (`0x18`)
- **EXT-14 connector pins 8/10** (Caps plug in here)

**Implication:** any Cap that uses I²C must avoid those three addresses.

---

## SoC details — confirmed

- **MCU:** ESP32-S3FN8 (no PSRAM in the FN8 variant)
- **Flash:** 8 MB, QIO mode, partitioned via `huge_app.csv` in our build (no OTA slot — single 6.4 MB app partition)
- **Confirmed in `platformio.ini`:** `-DBOARD_HAS_PSRAM=0` — correct.

**Implications:**

- All buffers must come from the ~327 KB of internal SRAM. We're at 84 KB used out of 327 KB → 243 KB free. A `.pcap` ring buffer of even 64 KB is fine; a multi-MB capture must spill to SD.
- No OTA today. v1.0 release flow is: M5Burner image or `pio run -t upload` over USB. Adding OTA means re-partitioning to halve the app slot — defer.

---

## Board ID — confirmed correct

`platformio.ini` uses `board = esp32-s3-devkitc-1` with `-DARDUINO_USB_CDC_ON_BOOT=1` and explicit pin overrides via M5Unified autodetect.

**Why this is correct:** there is no `m5stack-cardputer-adv` board ID in mainline `platform-espressif32`. M5Unified's runtime board-detection (M5GFX.cpp:1865-1869 — checks G5/G6 pull behavior + I²C device responses) reliably identifies the board as `board_M5CardputerADV` regardless of the build profile. So we get the right pins and peripherals at runtime even from the generic devkit profile.

The original 18-doc plan suggested `board = m5stack_core2` or `board = esp32dev` — both wrong. This was already corrected in v0.1.

---

## WiFi + BLE coexistence

ESP-IDF supports concurrent WiFi STA + BLE on the ESP32-S3 — but with constraints. Things to verify on hardware in Phase 0:

- **2.4 GHz time-slicing:** WiFi and BLE share one radio. Sustained WiFi monitor mode + active BLE GATT may drop packets in either direction. We expect to use them concurrently in v0.2 (Pineapple over WiFi + TH-D75 over BLE), so this needs measurement.
- **Memory:** BLE controller takes ~50 KB SRAM minimum. Combined with WiFi's ~80 KB, we lose ~130 KB before the app starts. Consistent with our 84 KB current allocation — there's still headroom but watch it.
- **Coexistence config:** `CONFIG_BT_CTRL_COEX` flags need to be on in `sdkconfig`. The Arduino-IDF default has them on; our build inherits this — verify no override breaks it.

**Action item:** add a "WiFi + BLE concurrent stress test" to the Phase 0 hardware bring-up checklist. Pair Pineapple (WiFi STA) and TH-D75 (BLE Central) at the same time and log packet loss.

---

## Phase 0 hardware-bring-up checklist (updated)

When the ADV is in hand:

1. Flash v0.1 firmware via `pio run -e cardputer_adv -t upload`.
2. Power on; verify the hinomaru-koi splash renders correctly (display orientation, colors, anti-aliasing).
3. Open `KeyTestApp` from `System` category. Press every key in the 4×14 matrix. Confirm the logical row/col reported matches the documented layout in `docs/HARDWARE.md` (and the M5 keymap reference). **Any off-by-one is a keymap bug; fix in `src/drivers/AdvKeymap.hpp`.**
4. Open `ToneApp` (in Fun) — confirm M5.Speaker plays the test tone clearly.
5. Open `Pomodoro` — let a 5-second test work-cycle finish; confirm the alarm tone fires.
6. Open `Wifi` (now under WiFi category in launcher). Connect to a known AP. Verify NTP auto-sync makes ClockApp's TimeOfDay show correct local time.
7. Reboot. Verify auto-connect kicks in without prompting.
8. Open `Settings` → change Timezone. Open Clock → confirm displayed time updates.
9. Open `Mic` (in Tools) — confirm FFT bands respond to nearby sound.
10. Open `IR` (in Tools) — point at a TV, confirm power-toggle works.
11. **Concurrency stress:** open Wifi to connect, then while WiFi is active, scan for BLE devices in Ble. Both should work; note any disconnects.
12. **Battery / charging:** unplug, leave on for 10 min on the launcher; confirm `BATT %` decreases. Plug back in; confirm it recovers.
13. **SD writes:** create a Note in Notes, save (Tab). Power-cycle. Verify it loads back.
14. Pick up `pins.hpp` change: add `kKbdInt = 11`. (No code change needed yet — wiring INT-driven keyboard polling is a v0.2 polish task.)

If steps 1–13 pass, v0.1 is hardware-validated and we're cleared for v0.2.

---

## TODO before next phase

- [x] Cross-check pins.hpp vs M5Unified — no errors found
- [x] Cross-check docs vs official M5 product page — 1 minor doc fix
- [x] Add `kKbdInt = 11` to `pins.hpp` (constant in tree at pins.hpp:13; INT-driven polling is a v0.2 polish task)
- [x] Fix EXT-14 connector pin labels in HARDWARE.md (table now shows physical two-column layout)
- [ ] Phase 0 hardware bring-up (gated on physical device)

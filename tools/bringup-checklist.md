# Yui — Phase 0 Hardware Bring-Up Checklist

Print this page. Plug in the M5Cardputer ADV. Work top to bottom. Tick each box as you go. **If any step fails, stop and file an issue before continuing** — most failures cascade.

**Build under test:** ******\*\*\*\*******\_\_\_******\*\*\*\******* (e.g. `f16b43e`)
**Date:** ******\*\*\*\*******\_\_\_******\*\*\*\*******
**Tester:** ******\*\*\*\*******\_\_\_******\*\*\*\*******

---

## Pre-flight

- [ ] **0.1** USB-C cable connects to a port that enumerates (not power-only). Linux: `dmesg | tail` shows `cdc_acm` or `ttyACM0`. macOS: `ls /dev/tty.usbmodem*` shows the device.
- [ ] **0.2** `~/.platformio/penv/bin/pio device list` shows the port (typically `/dev/ttyACM0`).
- [ ] **0.3** Battery is at ≥20% (otherwise tests 12-13 are noisy).

---

## 1. Flash the release image

- [ ] **1.1** Run `tools/pack-release.sh` — `release/yui-<hash>-cardputer_adv.bin` exists.
- [ ] **1.2** Flash:
  ```
  ~/.platformio/penv/bin/python3 ~/.platformio/packages/tool-esptoolpy/esptool.py \
    --chip esp32s3 -p /dev/ttyACM0 -b 921600 \
    write_flash --flash_mode qio --flash_freq 80m --flash_size 8MB \
    0x0 release/yui-<hash>-cardputer_adv.bin
  ```
- [ ] **1.3** "Hash of data verified" appears at the end of the flash output.
- [ ] **1.4** Device reboots automatically. (If not, press the reset button on the side.)

**Expected:** screen lights up within ~2 seconds.
**Abort if:** flash hangs, hash mismatch, device doesn't enumerate after flash.

---

## 2. Splash render

- [ ] **2.1** Hinomaru-koi splash renders: red Braille koi on white background.
- [ ] **2.2** Sweeping bright-red highlight column moves across the koi.
- [ ] **2.3** Version string is legible at the bottom-right.
- [ ] **2.4** Display orientation is correct (text reads left-to-right when keyboard faces you).
- [ ] **2.5** No visible tearing / dead pixels / color channel inversion.

**Abort if:** display blank or inverted (likely SPI pin mismatch — check `pins.hpp`).

---

## 3. Launcher categories

- [ ] **3.1** After splash, launcher shows category list: **Tools / Fun / WiFi / Ble / Radio / System**.
- [ ] **3.2** Status bar at bottom shows BATT% + RSSI/UPTIME.
- [ ] **3.3** Up/Down arrows move the cursor; selected row is highlighted.
- [ ] **3.4** Enter drills into a category; Esc returns to category list.

**Abort if:** launcher doesn't render or arrow keys do nothing (keymap or shell bug).

---

## 4. KeyTestApp — full keymap matrix

Open **System → KeyTest**.

- [ ] **4.1** Press every key in the 4×14 ADV matrix. Each press shows the logical row/col.
- [ ] **4.2** All 56 keys produce a unique (row, col).
- [ ] **4.3** Shift / Fn / Ctrl / Alt modifier flags appear when held.
- [ ] **4.4** No "stuck" keys (release should clear the displayed event).

**Mark mismatches:** any key whose physical position doesn't match the logical (row, col) is a bug in `src/drivers/AdvKeymap.hpp`. Note which keys, fix later.

---

## 5. Audio out — ToneApp

Open **Fun → Tone**.

- [ ] **5.1** Default 440 Hz tone plays through the onboard speaker, audible at arm's length.
- [ ] **5.2** Cycle through all 4 presets (Tab) — each is clearly different in pitch.
- [ ] **5.3** No buzzing, distortion, or DC offset click on tone-on / tone-off.

**Abort if:** silent (likely I²S pins or codec init failure — check `pins.hpp` lines for I²S + ES8311 address `0x18`).

---

## 6. Pomodoro alarm — full audio chain

Open **Fun → Pomodoro**.

- [ ] **6.1** Set work duration to 5 seconds (Up/Down adjusts when in setup).
- [ ] **6.2** Start the cycle. After 5s, alarm tone fires.
- [ ] **6.3** Break-period auto-starts; another 5s elapses, second alarm fires.

---

## 7. WifiApp — connect + persist

Open **WiFi → Wifi**.

- [ ] **7.1** Scan results populate within 5-10 s.
- [ ] **7.2** Pick a known AP. Enter passphrase. Status transitions: `Scanning → Connecting → Connected`.
- [ ] **7.3** Status bar RSSI shows a non-zero dBm value.
- [ ] **7.4** Note the IP from SysinfoApp (System → Sysinfo).

---

## 8. NTP + ClockApp

Open **Tools → Clock**.

- [ ] **8.1** Tab to **TimeOfDay** mode.
- [ ] **8.2** Displayed time matches local wallclock (within 1 s) — confirms NTP sync ran.
- [ ] **8.3** Stopwatch + Timer modes still function.

---

## 9. Reboot persistence

- [ ] **9.1** Power-cycle (unplug USB, plug back in).
- [ ] **9.2** Splash → launcher (no first-time prompts).
- [ ] **9.3** WifiApp shows `Connected: <ssid>` without retyping the passphrase (NVS persistence works).
- [ ] **9.4** ClockApp TimeOfDay shows synced time within ~10 s of boot.

**Abort if:** WiFi or settings don't persist (NVS init or `Esp32Storage` bug).

---

## 10. Settings → Timezone

Open **System → Settings**.

- [ ] **10.1** Timezone row visible. Pick a different TZ (use Up/Down to cycle presets).
- [ ] **10.2** Open ClockApp TimeOfDay. Displayed time has shifted by the expected offset.
- [ ] **10.3** Power-cycle. New TZ persists.

---

## 11. MicApp — input chain

Open **Tools → Mic**.

- [ ] **11.1** VU bar reacts to ambient sound.
- [ ] **11.2** Snap fingers near the device — at least one of the 8 FFT bands spikes.
- [ ] **11.3** Whistle a steady tone — band corresponding to the pitch is brightest.

---

## 12. IrRemoteApp

Open **Tools → IR**.

- [ ] **12.1** Point at a TV. Pick Power preset. Tab to fire.
- [ ] **12.2** TV responds (turns on/off).
- [ ] **12.3** Try at 2-3 m distance — confirms IR LED has reasonable throw.

---

## 13. WiFi + BLE concurrency stress

Open **WiFi → Wifi** (still connected from step 7), then **without disconnecting** open **Ble → Ble** in another launcher entry.

- [ ] **13.1** BLE scan runs while WiFi STA is connected.
- [ ] **13.2** WiFi connection survives the BLE scan (status bar RSSI doesn't drop to 0).
- [ ] **13.3** Check SysinfoApp: heap-free didn't drop below 60 KB.

**Note packet drops or disconnects** — these inform whether v1.0 needs to serialize radio access by category.

---

## 14. Battery + charging

- [ ] **14.1** Unplug USB. Sit on launcher 10 minutes.
- [ ] **14.2** BATT% shows a measurable decrease (>1 %).
- [ ] **14.3** Plug USB back in. Battery indicator shows charging state.
- [ ] **14.4** Sysinfo `BAT V` reads >3.7 V on charge.

---

## 15. SD persistence

- [ ] **15.1** Open **Tools → Notes**. Type a few lines. Tab to save.
- [ ] **15.2** Power-cycle.
- [ ] **15.3** Reopen Notes. Saved content loads back.
- [ ] **15.4** Open **Tools → Files**. Browse to `/yui-notes.txt` — file is present.

---

## Sign-off

If all boxes above are ticked, v0.3 is **hardware-validated**. Move on to:

- [ ] bb-link bridge pairing — see `docs/protocols/KENWOOD_THD75.md`
- [ ] 24-hour soak test — see `tools/soak-monitor.py`

Tester signature: ******\*\*\*\*******\_\_\_******\*\*\*\******* Date: \***\*\_\_\_\_\*\***

---

## Failure log

| Step | Symptom | Hypothesis | Fix commit |
| ---- | ------- | ---------- | ---------- |
|      |         |            |            |
|      |         |            |            |
|      |         |            |            |

# Yui — Phase 0 Hardware Bring-Up Checklist

Print this page. Plug in the M5Cardputer ADV. Work top to bottom. Tick each box as you go. **If any step fails, stop and file an issue before continuing** — most failures cascade.

**Build under test:** **\*\***\*\*\*\***\*\***\_\_\_**\*\***\*\*\*\***\*\*** (e.g. `f16b43e`)
**Date:** **\*\***\*\*\*\***\*\***\_\_\_**\*\***\*\*\*\***\*\***
**Tester:** **\*\***\*\*\*\***\*\***\_\_\_**\*\***\*\*\*\***\*\***

---

## Pre-flight

- [ ] **0.1** USB-C cable connects to a port that enumerates (not power-only). Linux: `dmesg | tail` shows `cdc_acm` or `ttyACM0`. macOS: `ls /dev/tty.usbmodem*` shows the device.
- [ ] **0.2** `~/.platformio/penv/bin/pio device list` shows the port (typically `/dev/ttyACM0`).
- [ ] **0.3** Battery is at ≥20% (otherwise tests 9-10 are noisy).

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

- [ ] **3.1** After splash, launcher shows category list: **Radio / WiFi / Bluetooth / Tools / System / Fun**.
- [ ] **3.2** Status strip shows BATT% + Hydra cap badge (`H+`/`H~`/`H-`) + clock/uptime.
- [ ] **3.3** Up/Down arrows move the cursor; selected row is highlighted.
- [ ] **3.4** Enter drills into a category; Esc returns to category list.

**Abort if:** launcher doesn't render or arrow keys do nothing (keymap or shell bug).

---

## 4. WifiApp — connect + persist

Open **WiFi → Wifi**.

- [ ] **4.1** Scan results populate within 5-10 s.
- [ ] **4.2** Pick a known AP. Enter passphrase. Status transitions: `Scanning → Connecting → Connected`.
- [ ] **4.3** Status bar RSSI shows a non-zero dBm value.
- [ ] **4.4** Note the IP from SysinfoApp (System → Sysinfo).

---

## 5. NTP + ClockApp

Open **System → Clock**.

- [ ] **5.1** Tab to **TimeOfDay** mode.
- [ ] **5.2** Displayed time matches local wallclock (within 1 s) — confirms NTP sync ran.
- [ ] **5.3** Stopwatch + Timer modes still function.

---

## 6. Reboot persistence

- [ ] **6.1** Power-cycle (unplug USB, plug back in).
- [ ] **6.2** Splash → launcher (no first-time prompts).
- [ ] **6.3** WifiApp shows `Connected: <ssid>` without retyping the passphrase (NVS persistence works).
- [ ] **6.4** ClockApp TimeOfDay shows synced time within ~10 s of boot.

**Abort if:** WiFi or settings don't persist (NVS init or `Esp32Storage` bug).

---

## 7. Settings → Timezone

Open **System → Settings**.

- [ ] **7.1** Timezone row visible. Pick a different TZ (use Up/Down to cycle presets).
- [ ] **7.2** Open ClockApp TimeOfDay. Displayed time has shifted by the expected offset.
- [ ] **7.3** Power-cycle. New TZ persists.

---

## 8. IrRemoteApp

Open **Tools → IR**.

- [ ] **8.1** Point at a TV. Pick Power preset. Tab to fire.
- [ ] **8.2** TV responds (turns on/off).
- [ ] **8.3** Try at 2-3 m distance — confirms IR LED has reasonable throw.

---

## 9. WiFi + BLE concurrency stress

Open **WiFi → Wifi** (still connected from step 4), then **without disconnecting** open **Bluetooth → Ble**.

- [ ] **9.1** BLE scan runs while WiFi STA is connected.
- [ ] **9.2** WiFi connection survives the BLE scan (status bar RSSI doesn't drop to 0).
- [ ] **9.3** Check SysinfoApp: heap-free didn't drop below 60 KB.

**Note packet drops or disconnects** — these inform whether the firmware needs to serialize radio access by category.

---

## 10. Battery + charging

- [ ] **10.1** Unplug USB. Sit on launcher 10 minutes.
- [ ] **10.2** BATT% shows a measurable decrease (>1 %).
- [ ] **10.3** Plug USB back in. Battery indicator shows charging state.
- [ ] **10.4** Sysinfo `BAT V` reads >3.7 V on charge.

---

## 11. Hydra RF cap — CC1101 + nRF24 detection

Plug in the **Pingequa Hydra Cap 424** before powering up.

- [ ] **11.1** Open **System → HydraStatus**. Both `CC1101` and `nRF24` rows show `present`.
- [ ] **11.2** Launcher status strip shows `H+`. Pull the cap, reboot — strip shows `H-`.
- [ ] **11.3** With cap seated, open every CC1101 app (Radio → SubGhz Scan / Capture / Replay / Brute / Jammer / RollJam) — none crash, each shows the connection icon as **connected**.
- [ ] **11.4** Open every nRF24 app (Bluetooth → Nrf24 Scan / Nrf24 Jammer / Mousejack) — none crash, each shows the connection icon as **connected**.
- [ ] **11.5** Pull the cap (without rebooting) and reopen any RF app — each surfaces a "module not detected" dialog rather than crashing, and the connection icon flips to **disconnected**.

---

## 12. SD persistence

- [ ] **12.1** Open **Tools → Files**. Confirm root filesystem mounts.
- [ ] **12.2** From Radio → SubGhz Capture, capture a signal and save. File appears under `/captures/`.
- [ ] **12.3** Power-cycle. Capture file is still present in Files.

---

## Sign-off

If all boxes above are ticked, the firmware is **hardware-validated**. Move on to:

- [ ] bb-link bridge pairing — see `docs/protocols/KENWOOD_THD75.md`
- [ ] 24-hour soak test — see `tools/soak-monitor.py`

Tester signature: **\*\***\*\*\*\***\*\***\_\_\_**\*\***\*\*\*\***\*\*** Date: \***\*\_\_\_\_\*\***

---

## Failure log

| Step | Symptom | Hypothesis | Fix commit |
| ---- | ------- | ---------- | ---------- |
|      |         |            |            |
|      |         |            |            |
|      |         |            |            |

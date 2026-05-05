# Installing Yui

Three paths, easiest first.

1. [Web flasher](#web-flasher) — open a URL, click Install. No tooling required.
2. [esptool](#esptool) — one command if you have Python.
3. [Build from source](#build-from-source) — for hacking on the firmware.

After flashing jump to [first boot](#first-boot).

## Web flasher

Open <https://leingangzj.github.io/yui> in Chrome, Edge, Brave, or any Chromium-based browser on Windows / macOS / Linux. Plug the Cardputer ADV into a USB-C port that carries data (a charge-only cable won't enumerate as serial). Click **Install**, pick the serial port from the prompt, wait about 90 seconds. The device reboots into the splash on its own.

The page uses the [Web Serial API][ws], which Firefox and Safari don't implement. Try one of the other paths there.

[ws]: https://developer.mozilla.org/docs/Web/API/Web_Serial_API

## esptool

Grab `yui-vX.Y-cardputer_adv.bin` from the [latest release][rel] — it's a single merged image you write to flash offset `0x0`.

```bash
pip install esptool
python3 -m esptool --chip esp32s3 -p /dev/ttyACM0 -b 921600 \
  write_flash --flash_mode qio --flash_freq 80m --flash_size 8MB \
  0x0 yui-v1.2.0-cardputer_adv.bin
```

[rel]: https://github.com/leingangzj/yui/releases/latest

| OS      | Typical port        |
| ------- | ------------------- |
| Linux   | `/dev/ttyACM0`      |
| macOS   | `/dev/cu.usbmodem*` |
| Windows | `COM3` (or similar) |

If `esptool` reports a sync error, hold the Cardputer's `G0` button while plugging in to force download mode. Newer revisions auto-enter download mode when needed.

## Build from source

For unreleased commits, hacking on the firmware, or unsupported platforms.

**Prereqs:** Python 3.9+, [PlatformIO Core][pio], a USB-C data cable, and a Cardputer ADV (not the original Cardputer — different hardware).

[pio]: https://platformio.org/install/cli

```bash
git clone https://github.com/leingangzj/yui.git
cd yui

pio test -e native            # host tests, ~4s, no hardware
pio run -e cardputer_adv      # build firmware (~90s)
pio run -e cardputer_adv -t upload -t monitor
```

First build pulls ~1 GB of toolchain (Arduino-ESP32, M5Unified, M5GFX, NimBLE, RadioLib, etc.); subsequent runs are incremental.

To produce the same single-image .bin the web flasher uses:

```bash
tools/pack-release.sh v1.2.0
# -> release/yui-v1.2.0-cardputer_adv.bin
# -> release/yui-v1.2.0-manifest.json
```

## First boot

### WiFi

**WiFi → WiFi** from the launcher. Scan, pick the AP, type the passphrase, hit Enter. Credentials persist in NVS so the device auto-connects on subsequent boots. NTP runs once the link is up.

### Timezone

**System → Settings**, arrow down to Timezone, Left/Right cycles through nine POSIX presets (UTC, US Eastern/Central/Mountain/Pacific, UK, Central Europe, Japan, Sydney). Persisted.

### NTP server (optional)

Same Settings page. Default is `pool.ntp.org`; Cloudflare, Google, and NIST are also in the picker.

### Quick health check

**System → Self-Test** runs all 14 hardware probes (display, keyboard, SD, NVS, WiFi, BLE, IR, IMU, mic, speaker, clock, CC1101, nRF24, registry headroom) and shows pass/skip/fail per row. Press Enter, watch the verdict. Useful right after flashing a fresh board.

### Kenwood TH-D75 + bb-link bridge (optional)

Required for any ham-radio feature (APRS, GPS log, SatTracker CAT, RemoteHead). You need:

- A TH-D75
- A separate **original** ESP32 (not the S3) flashed with [bb-link][bblink] firmware. The bridge talks BT-Classic SPP to the radio and BLE GATT to the Cardputer.

[bblink]: https://github.com/islandmagic/bb-link

Pair the bridge to the radio once via bb-link's instructions (BT-Classic, PIN `0000`). Then in Yui:

1. **System → bb-link Probe**, Enter to validate the three-phase chain (BT scan → BLE connect → GATT NUS). Confirm `BridgeReady` before going further.
2. **Radio → Kenwood**. Status flips to `CommandMode` once connected. Enter refreshes ID / freq / mode.
3. To make the pairing persistent, write the bridge's BLE MAC into NVS key `radio.mac` via serial. Yui reconnects without scanning on next boot.

### WiFi Pineapple (optional)

Until a config UI ships, set these NVS keys via the serial monitor:

| Key       | Value                            |
| --------- | -------------------------------- |
| `pa.host` | Pineapple IP, e.g. `172.16.42.1` |
| `pa.port` | `1471` (default)                 |
| `pa.user` | `root`                           |
| `pa.pass` | your Pineapple password          |
| `pa.ssid` | the Pineapple's WiFi SSID        |
| `pa.psk`  | the Pineapple's WiFi passphrase  |

Open **WiFi → Pineapple** and Yui swaps association from your home WiFi to the Pineapple's AP for the duration of the app, then restores on exit. Expect a 2–3 second blip in any WiFi-dependent app during the swap.

### APRS callsign (optional, only if you'll TX)

Set `tx.call` to your callsign (no SSID) and `tx.ssid` to a number 0–15. Yui won't transmit anything until both are set.

### Lab / Faraday Mode (optional)

If you're working in a Faraday-shielded room and want the aggressive RF apps to actually be aggressive, **System → Settings → Faraday Mode** flips it on. The launcher status strip grows a red `LAB` badge. Off-state is bit-for-bit identical to the politeness defaults.

If you set a lab GPS location (`sys.lab_lat` / `sys.lab_lon` in NVS, micro-degrees), Faraday Mode refuses to enable when a GPS fix is available and you're outside a 50 m radius. Belt-and-suspenders against forgetting to disable outdoors.

## Troubleshooting

**"Failed to connect to ESP32: Timed out waiting for packet header"** — hold `G0`, plug USB-C, re-run the flash command. Some revisions need manual download-mode entry.

**Splash renders but the launcher is upside-down** — `M5GFX`'s autodetect picked the wrong rotation. File an issue with the boot log (`pio device monitor`).

**SelfTestApp reports a row as Fail** — check the diagnostic string under that row. Display/Keyboard/Clock failures usually mean the M5Unified probe didn't ID the board (try `M5.config()` adjustments). Cap-missing on CC1101/nRF24 is normal without the Hydra cap.

**"no NTP sync" stays on the Sysinfo Time row** — WiFi has to be up first (Sysinfo shows the IP when it is). Then verify the NTP server in Settings is reachable. Some corporate networks block UDP/123.

**Pineapple connects but recon never completes** — Pineapple firmware versions vary; response shape can differ. Open an issue with the body of `/api/recon/status` from a known good scan.

**TH-D75 GPS row is empty after pairing** — the radio's `GP` (GPS data output) menu must be on. Bridge passes whatever the radio sends.

**`DeauthApp(Native)` shows "TX failed (libnet?)"** — the firmware was built without the `freedom_override.c` source path. Confirm `src/lib_extra_override/freedom_override.c` exists and `-Wl,-zmuldefs` is in `platformio.ini`. Verify the override is linked: `xtensa-esp32s3-elf-gcc-nm -S firmware.elf | grep ieee80211_freedom_inside_cb` should show size `0x07`.

## Uninstalling

```bash
pio run -e cardputer_adv -t erase
# or
python3 -m esptool --chip esp32s3 erase_flash
```

Wipes flash. Device goes back to blank state.

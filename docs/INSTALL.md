# Installing Yui

Three ways to flash, in order of friction:

1. **Web flasher** — open a URL in Chrome / Edge / Brave on a desktop OS,
   plug in the device, click Install. No install required.
2. **esptool from a Python install** — one command if you already have
   Python on the box.
3. **Build from source** — for contributors and people who want to run
   off `main`.

After flashing, jump to [First-run setup](#first-run-setup) below.

## Path 1 — Web flasher

Go to **https://leingangzj.github.io/yui** in Chrome, Edge, Brave, or any
recent Chromium-based browser on Windows / macOS / Linux. Plug the
Cardputer ADV into a USB-C port that carries data (a charge-only cable
won't enumerate a serial device). Click **Install**, pick the serial
port from the prompt, wait about 90 seconds. The device reboots into
the splash on its own.

Why this works: the page uses the
[Web Serial API](https://developer.mozilla.org/docs/Web/API/Web_Serial_API),
which is supported in any Chromium-based browser. Firefox and Safari
do not implement it, so try one of the other paths there.

## Path 2 — esptool

Grab the prebuilt image from the
[latest release](https://github.com/leingangzj/yui/releases/latest) — the
asset named `yui-vX.Y-cardputer_adv.bin` is a single merged image you can
write to flash offset `0x0`.

```bash
pip install esptool   # if you don't already have it
python3 -m esptool --chip esp32s3 -p /dev/ttyACM0 -b 921600 \
  write_flash --flash_mode qio --flash_freq 80m --flash_size 8MB \
  0x0 yui-v0.3-cardputer_adv.bin
```

Substitute the right port for your OS:

| OS      | Typical port        |
| ------- | ------------------- |
| Linux   | `/dev/ttyACM0`      |
| macOS   | `/dev/cu.usbmodem*` |
| Windows | `COM3` (or similar) |

Hold the Cardputer's `G0` button while plugging in if `esptool` reports
a sync error — that forces download mode. Newer revisions auto-enter
download mode when needed.

## Path 3 — Build from source

Use this if you want to run an unreleased commit, hack on the firmware,
or your platform isn't covered by the binary.

### Prerequisites

- Python 3.9+
- [PlatformIO Core](https://platformio.org/install/cli) — `pip install platformio`
- USB-C data cable
- An M5Stack Cardputer ADV (not the original Cardputer — different hardware)

### Clone, test, build, flash

```bash
git clone https://github.com/leingangzj/yui.git
cd yui

# Run the host-side tests first to confirm the toolchain is sane.
pio test -e native

# Build firmware. The first run pulls ~1 GB of toolchain + libraries
# (M5Unified, M5GFX, IRremoteESP8266, NimBLE, ESP-IDF, Arduino-ESP32);
# subsequent runs are incremental.
pio run -e cardputer_adv

# Flash + drop into a serial monitor at 115200 baud.
pio run -e cardputer_adv -t upload -t monitor
```

To produce the same single-image .bin the web flasher uses:

```bash
tools/pack-release.sh v0.3
# -> release/yui-v0.3-cardputer_adv.bin
# -> release/yui-v0.3-manifest.json
```

## First-run setup

### WiFi

Open **WiFi → Wifi** from the launcher. The app scans, you pick the AP,
type the passphrase, hit Enter. Credentials are persisted in NVS so the
device auto-connects on subsequent boots. NTP runs once the link is up.

### Timezone

**System → Settings**, arrow down to Timezone, Left/Right cycles through
nine POSIX presets (UTC, US Eastern/Central/Mountain/Pacific, UK, Central
Europe, Japan, Sydney). Persisted.

### NTP server (optional)

Same Settings page. Default is `pool.ntp.org`; Cloudflare, Google, and
NIST are also in the picker.

### Kenwood TH-D75 + bb-link bridge (optional)

Required for any ham-radio feature (APRS, GPS log, SatTracker CAT
control, RemoteHead). You need:

- A TH-D75
- A separate ESP32 (the **original**, not the S3) flashed with the
  [bb-link firmware](https://github.com/islandmagic/bb-link). The
  bridge talks BT-Classic SPP to the radio and BLE GATT to the
  Cardputer.

Pair the bridge to the radio once via bb-link's own instructions
(BT-Classic, PIN `0000`). Then in Yui:

1. **Radio → Kenwood**, Tab to scan for `B.B. Link` over BLE.
2. After connect, status flips to `CommandMode`. Enter refreshes
   ID / freq / mode.
3. To make the pairing persistent, write the bridge's BLE MAC into NVS
   key `radio.mac` via serial. Yui will reconnect to that MAC on next
   boot without scanning.

The bridge exists because the Cardputer's ESP32-S3 doesn't do
Bluetooth Classic, and the TH-D75 only speaks Classic SPP.

### WiFi Pineapple (optional)

Until a configuration UI ships, set these NVS keys via the serial
monitor:

| Key       | Value                            |
| --------- | -------------------------------- |
| `pa.host` | Pineapple IP, e.g. `172.16.42.1` |
| `pa.port` | `1471` (default)                 |
| `pa.user` | `root`                           |
| `pa.pass` | your Pineapple password          |
| `pa.ssid` | the Pineapple's WiFi SSID        |
| `pa.psk`  | the Pineapple's WiFi passphrase  |

When you open **WiFi → Pineapple**, Yui swaps association from your home
WiFi to the Pineapple's AP for the duration of the app, then restores
on exit. Expect a 2–3 second blip in any WiFi-dependent app during the
swap.

### APRS callsign (optional, only if you'll TX)

Set `tx.call` to your callsign (no SSID) and `tx.ssid` to a number
0–15. Yui won't transmit anything until both are set.

## Troubleshooting

### "Failed to connect to ESP32: Timed out waiting for packet header"

Hold `G0`, plug USB-C, then re-run the flash command. Some revisions
need the manual download-mode entry; auto-detect doesn't always fire
on the first try.

### Splash renders but the launcher is upside-down

`M5GFX`'s autodetect picked the wrong rotation. File an issue and
include the boot log (`pio device monitor`).

### KeyTestApp reports the wrong row/col for a key

The TCA8418 keymap drift. Compare with `docs/HARDWARE.md` ➝ "Keyboard
Matrix" and PR a corrected matrix in `src/drivers/AdvKeymap.hpp`.

### "no NTP sync" stays on the Sysinfo Time row

WiFi has to be up first (Sysinfo shows the IP when it is). Then verify
the NTP server in Settings is reachable from your network — some
corporate networks block UDP/123.

### Pineapple connects but recon never completes

Pineapple firmware versions vary, and the response shape can differ.
Open an issue with the body of `/api/recon/status` from a known good
scan.

### TH-D75 GPS row is empty even after pairing

Make sure the radio's `GP` (GPS data output) menu option is **on**.
The bridge passes through whatever the radio sends; if the radio isn't
emitting NMEA, neither is the bridge.

## Uninstalling

`pio run -e cardputer_adv -t erase` wipes flash, or `python3 -m esptool
--chip esp32s3 erase_flash`. The device goes back to a blank state and
will boot the next firmware you flash.

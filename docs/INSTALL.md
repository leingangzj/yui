# Installing Yui

Two paths: end-user (M5Burner image, ~5 minutes) and developer (build from
source). M5Burner is the recommended path once v1.0 is published; until then,
build from source.

## Path 1 — M5Burner (coming with v1.0)

1. Plug the Cardputer ADV into your computer with a USB-C cable.
2. Download and run [M5Burner](https://docs.m5stack.com/en/download).
3. Search for "Yui" in the **Cardputer ADV** filter. The most recent release
   tag is the right one.
4. Click **Burn**. M5Burner will download the firmware bundle, set the
   correct board profile, erase, and flash. Takes ~30 seconds.
5. The device reboots into the hinomaru-koi splash → first-run setup.

## Path 2 — Build from source

### Prerequisites

- Python 3.9+
- [PlatformIO Core](https://platformio.org/install/cli) (`pip install platformio`)
- USB-C cable
- An M5Stack Cardputer ADV (not the original Cardputer — they have different
  hardware)

### Clone and build

```bash
git clone https://github.com/leingangzj/yui.git
cd yui

# Run host-side tests first to make sure the toolchain is working.
pio test -e native

# Build firmware. First run pulls ~1 GB of toolchain + libs (M5Unified,
# M5GFX, IRremoteESP8266, NimBLE, ESP-IDF + Arduino ESP32). Goes faster
# after that.
pio run -e cardputer_adv
```

### Flash

Hold the Cardputer's `G0` button while plugging in the USB-C cable to enter
download mode (newer revisions auto-enter when needed; if `pio` reports a
sync error, hold `G0` and try again).

```bash
pio run -e cardputer_adv -t upload -t monitor
```

The serial monitor at 115200 baud shows the boot log; the device runs
straight to the splash + launcher.

## First-run setup

### WiFi

Open the launcher (the device's home screen). Drill into **WiFi** category
→ open **WiFi** → it scans, you select your AP, type the passphrase, Enter.
Yui persists the credentials in NVS (Non-Volatile Storage) so it auto-
connects on subsequent boots. Auto-NTP fires after the link comes up so
the wallclock and SatTracker have accurate time.

### Timezone

**System → Settings**: arrow down to Timezone, Left/Right cycles through
nine POSIX TZ presets (UTC, US E/C/M/P, UK, Central Europe, Japan, Sydney).
Persisted.

### NTP server (optional)

Same Settings page; defaults to `pool.ntp.org`. Cloudflare / Google / NIST
also available.

### TH-D75 + bb-link bridge (optional, for ham radio features)

You'll need a separate ESP32 (original — NOT S3) running the
[bb-link](https://github.com/islandmagic/bb-link) firmware. Pair the bridge
to your TH-D75 once via the bb-link instructions (BT-Classic, PIN 0000).

In Yui:

1. **Radio → Kenwood** — press **Tab** to scan for "B.B. Link" via BLE.
2. After connect, status flips to "ready"; Enter refreshes ID/freq/mode.
3. To make the connection persistent, set NVS `radio.mac` to the bridge's
   BLE MAC via the serial monitor: `pio device monitor` then type the
   appropriate command (see USING.md).

The reason for the bridge: the Cardputer ADV's ESP32-S3 doesn't speak
Bluetooth Classic, but the TH-D75 only speaks BT-Classic SPP. The bridge
translates. ~$5 of hardware buys you APRS, GPS feed, SatTracker
Doppler-tuning, and CAT control of the radio.

### WiFi Pineapple (optional, for WiFi pentest features)

Configure these NVS keys via serial monitor (until v1.0 adds a UI):

| Key       | Value                            |
| --------- | -------------------------------- |
| `pa.host` | Pineapple IP, e.g. `172.16.42.1` |
| `pa.port` | `1471` (default)                 |
| `pa.user` | `root`                           |
| `pa.pass` | your Pineapple password          |
| `pa.ssid` | the Pineapple's WiFi SSID        |
| `pa.psk`  | the Pineapple's WiFi passphrase  |

When you open **WiFi → Pineapple**, Yui swaps association from your home
WiFi to the Pineapple's AP for the duration of the app, then restores on
exit. There will be a 2–3 second blip in WiFi-dependent apps during the
swap.

### Time-of-day callsign / GPS / etc.

For APRS TX, set `tx.call` (your callsign, no SSID) and `tx.ssid`
(0–15) in NVS.

## Troubleshooting

### "Failed to connect to ESP32: Timed out waiting for packet header"

Hold `G0`, plug USB-C, then run `pio run -e cardputer_adv -t upload`.

### Splash shows red bar but the launcher list is upside-down

The `M5GFX` autodetect gave us the wrong rotation. File a bug and include
the output of `pio device monitor` from boot.

### KeyTestApp shows row/col off-by-one

If a key shows up at the wrong logical position, the TCA8418 keymap derived
from the M5 schematic is off. Compare with `docs/HARDWARE.md` § "Keyboard
Matrix" and submit a PR with the corrected matrix in
`src/drivers/AdvKeymap.hpp`.

### "no NTP sync" stays on the Time row of Sysinfo

Verify WiFi is up first (System → Sysinfo shows the IP). Then verify the
NTP server in Settings is reachable from your network. Some corporate
networks block UDP/123.

### The Pineapple connects but recon scan never completes

Pineapple firmware versions vary; the response shape can differ. Open an
issue with the JSON body of `/api/recon/status` from a known scan.

### TH-D75 GPS row is empty even after pairing

Make sure the radio's `GP` (GPS data output) menu option is **on**. The
bridge passes through whatever the radio sends; if the radio isn't
emitting NMEA, neither is the bridge.

## Uninstalling

`pio run -e cardputer_adv -t erase` wipes flash. The Cardputer goes back to
factory state (M5Stack default firmware on next boot if it can find the
M5Burner default; otherwise blank).

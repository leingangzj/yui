# Using Yui

A reference for what's in the launcher and how to drive each app. Keys
follow Flipper-Zero conventions: arrow keys navigate, Enter activates,
Esc backs out, Tab is the secondary action, Fn+Enter is "armed-action"
for anything that TXes RF.

## Launcher

Two-level navigation:

1. **Categories** — Radio, WiFi, Bluetooth, Tools, System, Fun.
2. **Apps** — drill into a category to see its apps.

| Key       | Action                                |
| --------- | ------------------------------------- |
| Up / Down | navigate                              |
| Enter     | open category / launch app            |
| Esc       | back to category list (from app list) |

Status strip at the top shows battery %, current WiFi RSSI (or 0 when
disconnected), and either the wallclock (when NTP-synced) or uptime.

## Radio

| App            | What it does                                                                                                                                                                                                               |
| -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Kenwood**    | Status view for the TH-D75. Shows ID / VFO frequency / mode. Tab to (re)connect to the bb-link bridge using `radio.mac` from NVS. Enter refreshes.                                                                         |
| **APRS**       | RX-only station list. Auto-puts the radio in KISS mode on enter; logs each unique CALL[-SSID] with last-heard timestamp + position. Up to 32 stations; oldest evicted when full.                                           |
| **APRS TX**    | Compose an APRS text message. Reads your callsign from `tx.call` / `tx.ssid` NVS. Tab cycles To / Msg fields. Enter sends.                                                                                                 |
| **GPS**        | Live fix display from the radio's GNSS. Tab toggles GPX recording (~1 trackpoint every 5 sec, 64-point cap); second Tab writes `/yui-tracks/<unix>.gpx` to SD.                                                             |
| **Remote**     | TH-D75 VFO/freq/mode editor. Tab cycles Vfo/Freq/Mode fields; Up/Down adjusts; Enter writes.                                                                                                                               |
| **SatTracker** | Favorites list with next-pass countdown → detail (AOS/MAX/LOS) → live polar-plot view with Doppler-corrected freq writing to the TH-D75 every 2 sec. TLE source: `/yui-tles.txt` on SD if present, else built-in defaults. |

## WiFi

| App                 | What it does                                                                                                                                                                                                                                                              |
| ------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **WiFi**            | AP scan + connect + auto-reconnect. Fn+Backspace forgets stored creds.                                                                                                                                                                                                    |
| **Pineapple**       | Dashboard cards (CPU/mem/temp/clients/SSIDs) for the configured Pineapple. Tab refreshes.                                                                                                                                                                                 |
| **Recon**           | Drives a Pineapple recon scan. Enter starts a 30-sec scan, polls status every second, lands in Done when complete.                                                                                                                                                        |
| **Probes**          | Probe-request fingerprinting. Channel-hops 1..13 every 250 ms. Records (SSID, client MAC) → count. Up to 64 entries.                                                                                                                                                      |
| **Handshake**       | Passive WPA handshake capture from Cardputer onboard radio. Streams every mgmt+data frame to `/handshakes/cap-<unix>.pcap`.                                                                                                                                               |
| **Handshakes (PA)** | Read-only count of handshakes the Pineapple has captured.                                                                                                                                                                                                                 |
| **EvilTwin**        | Toggle PineAP rogue-AP mode + clear SSID pool. Fn+Enter to enable; Backspace to disable.                                                                                                                                                                                  |
| **Karma**           | Toggle Pineapple's Karma probe-responder. Fn+Enter to enable; Backspace to disable.                                                                                                                                                                                       |
| **Deauth**          | **Pineapple-driven deauth.** Tab arms; Up/Down picks channel; Fn+Enter fires. Tag the BSSID via NVS or future UI.                                                                                                                                                         |
| **Deauth!**         | **Native deauth.** Same controls; runs through Cardputer onboard radio. Requires patched libnet (see ROADMAP.md "Known TX caveats").                                                                                                                                      |
| **Beacon Flood**    | Rotating fake APs. Configurable SSID list (8 defaults). Tab toggles. Up/Down changes channel.                                                                                                                                                                             |
| **WPS Scan**        | Channel-hopping passive listener for beacons advertising the Microsoft WPS IE. APs with WPS=Yes are flagged.                                                                                                                                                              |
| **CaptivePortal**   | Cardputer becomes an open SoftAP with DNS-redirect-all + HTTP form. Captured POST bodies are stashed in a 16-slot ring; Tab flushes to `/captures.txt`. Fn+Enter starts; Backspace stops. SSID set via `cp.ssid`; HTML template via `cp.html` (or default phishing form). |

## Bluetooth

| App          | What it does                                                                                                                                    |
| ------------ | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| **BLE**      | Passive scan of nearby BLE devices.                                                                                                             |
| **BLE Spam** | Cycles three known nuisance payloads (Apple iBeacon shape, Samsung Easy Setup, Google Fast Pair placeholder). Off by default; Fn+Enter toggles. |
| **GATT**     | Connect to a BLE device by MAC. Walks services → drill into chars. View their UUIDs and properties.                                             |
| **BLE Jam**  | Rapid empty-payload BLE adv (~100/sec). 30-second auto-shutoff failsafe. Fn+Enter toggles.                                                      |

## Tools

| App            | What it does                                                                                                                                           |
| -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Calculator** | RPN with trig, deg/rad mode, π/e constants, 1-slot memory. `n=neg q=sqrt s=swap c=clear i=sin o=cos t=tan d=mode g=π e=e p=Mstore r=Mrecall k=Mclear`. |
| **Notes**      | Tiny line-oriented editor. Tab saves to `/yui-notes.txt`.                                                                                              |
| **Files**      | microSD browser. Enter on a file shows its content; Tab toggles hex view.                                                                              |
| **IR Remote**  | Universal IR blast.                                                                                                                                    |
| **Mic**        | 8-band FFT visualizer.                                                                                                                                 |
| **TV-B-Gone**  | NEC TV power codes (12 manufacturers). Tab starts; auto-stops when the table's exhausted.                                                              |
| **IMU**        | Bubble level (BMI270 accel).                                                                                                                           |
| **KeyTest**    | Hardware bring-up tool — shows which TCA8418 row/col was pressed.                                                                                      |

## System

| App          | What it does                                                                                                                                  |
| ------------ | --------------------------------------------------------------------------------------------------------------------------------------------- |
| **Clock**    | Stopwatch / Timer / Time-of-day modes. Tab cycles. Time-of-day reads `clock_.epoch_seconds()`; in TimeOfDay mode, Enter forces an NTP resync. |
| **Sysinfo**  | Battery, free heap, uptime, IP, RSSI, wallclock, GPS fix status.                                                                              |
| **Settings** | Brightness / Timezone / NTP server.                                                                                                           |
| **About**    | Version, build info.                                                                                                                          |

## Fun

| App                                                                 | What it does                               |
| ------------------------------------------------------------------- | ------------------------------------------ |
| Snake / Life / Draw / Calendar / Todo / Tone / Metronome / Pomodoro | Toys. Toggleable, mostly self-explanatory. |

## NVS keys reference

Yui persists configuration in the ESP32's NVS (Preferences) namespace
`yui`. Set these via serial monitor or a future Settings UI.

| Key          | Type | Purpose                                    |
| ------------ | ---- | ------------------------------------------ |
| `brightness` | int  | LCD brightness 0..100                      |
| `wifi.ssid`  | str  | Home WiFi SSID                             |
| `wifi.pass`  | str  | Home WiFi passphrase                       |
| `clock.tz`   | str  | POSIX TZ string                            |
| `clock.ntp`  | str  | NTP server hostname                        |
| `radio.mac`  | str  | bb-link bridge BLE MAC for auto-connect    |
| `radio.pin`  | str  | BT-Classic PIN (default `0000`)            |
| `tx.call`    | str  | Your callsign for APRS TX                  |
| `tx.ssid`    | int  | Your APRS SSID 0..15                       |
| `pa.host`    | str  | Pineapple host IP                          |
| `pa.port`    | int  | Pineapple REST port (default 1471)         |
| `pa.user`    | str  | Pineapple username                         |
| `pa.pass`    | str  | Pineapple password                         |
| `pa.ssid`    | str  | Pineapple WiFi SSID                        |
| `pa.psk`     | str  | Pineapple WiFi passphrase                  |
| `cp.ssid`    | str  | Captive Portal advertised SSID             |
| `cp.html`    | str  | Captive Portal HTML template (≤1024 bytes) |

## Files Yui writes to SD

| Path                          | Content                                  |
| ----------------------------- | ---------------------------------------- |
| `/yui-notes.txt`              | NotesApp content                         |
| `/yui-todo.txt`               | TodoApp items                            |
| `/yui-tles.txt`               | SatTracker TLE database (3-line records) |
| `/yui-tracks/<unix>.gpx`      | GpsApp trackpoints when recording        |
| `/yui-draw.pbm`               | DrawApp pixel grid                       |
| `/handshakes/cap-<unix>.pcap` | WifiHandshakeApp captures                |
| `/captures.txt`               | CaptivePortalApp form submissions        |

## Legal

Several apps transmit RF or interact with networks/devices around you.
**Use only on RF spectrum / networks / devices you own or have written
authorization to test.** TX of WPA deauth, BLE-spam, beacon flood,
captive-portal phishing, and the like against parties who have not
consented is a federal crime in the US (CFAA / Wiretap Act / FCC
Part 15) and equivalent elsewhere. Yui surfaces "use only on YOUR network"
hints in every app that does this; that's not legal advice, it's a
reminder to a future-you.

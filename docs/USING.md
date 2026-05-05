# Using Yui

What's in the launcher and how to drive each app. Keys follow Flipper-Zero conventions: arrows navigate, Enter activates, Esc backs out, Tab is the secondary action, Fn+Enter is "armed action" for anything that TXes RF.

## Launcher

Two-level navigation: pick a category, drill into apps.

| Key       | Action                     |
| --------- | -------------------------- |
| Up / Down | navigate                   |
| Enter     | open category / launch app |
| Esc       | back to category list      |

Status strip at the top shows: Hydra cap state (`H+` / `H~` / `H-`), `LAB` badge if Faraday Mode is on, battery %, WiFi RSSI, and either wallclock (NTP-synced) or uptime.

## Radio

| App                     | What it does                                                                                                                                                                                                 |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Kenwood**             | TH-D75 status. Shows ID / VFO / mode. Tab reconnects to the bb-link bridge using `radio.mac` from NVS. Enter refreshes.                                                                                      |
| **APRS**                | RX-only station list. Auto-puts the radio in KISS mode on entry; logs CALL[-SSID] with last-heard timestamp + position. Up to 32 stations, oldest evicted.                                                   |
| **APRS TX**             | Compose an APRS text message. Reads callsign from `tx.call` / `tx.ssid`. Tab cycles To / Msg fields, Enter sends.                                                                                            |
| **GPS**                 | Live fix display from the radio's GNSS. Tab toggles GPX recording (~1 trackpoint every 5s, 64-point cap); second Tab writes `/yui-tracks/<unix>.gpx`.                                                        |
| **Remote**              | TH-D75 VFO/freq/mode editor. Tab cycles fields, ^/v adjusts, Enter writes.                                                                                                                                   |
| **SatTracker**          | Favorites list with next-pass countdown → AOS/MAX/LOS → live polar plot with Doppler-corrected freq writes to the TH-D75 every 2s. TLEs from `/yui-tles.txt` if present, else built-in defaults.             |
| **Sub-GHz Scan**        | Spectrum sweep. ^/v changes band (315/433/868/915 MHz). Enter pauses. Smart dwell (5 samples/bin); auto-tune doubles dwell on quiet bins.                                                                    |
| **Sub-GHz Capt/Replay** | Tab cycles Read (live capture) ↔ Saved (file picker). In Read: </> tunes, Enter starts/stops record, file goes to `/sub/<freq>_<ts>.sub`. In Saved: ^/v navigates, Enter transmits via edge-toggle async TX. |
| **Sub-GHz Brute**       | 24-bit OOK fixed-code walker. Enter starts; pace is 100 ms/key (10 ms/key in Faraday Mode).                                                                                                                  |
| **Sub-GHz Jam**         | ^/v picks mode (CW / Noise / Sweep / Protocol / Reactive). Enter activates. Protocol/Reactive modes register a packet-IRQ and only fire CW when a target preamble is heard.                                  |
| **NRF24 Jam**           | Channel-flood / hop-jam on 2.4 GHz. ^/v changes channel, Enter starts.                                                                                                                                       |
| **Mousejack**           | Logitech Unifying scan + keystroke inject. Tab toggles Targets ↔ Generic Scan (2.4 GHz channel heatmap, the absorbed Nrf24Scan path). Inject pace 50 ms (2 ms in Faraday Mode).                              |
| **RollJam**             | Capture/jam state machine for rolling-code remotes. Stages: Idle → Jamming → Captured → Replayed.                                                                                                            |
| **RF Chaos**            | Lab-only RF stress orchestrator. Random-target fires across BLE / nRF24 / CC1101 / WiFi-monitor for N minutes. Refuses to arm without Faraday Mode. Logs to `/rfchaos/<ts>.log`.                             |
| **Hydra Status**        | Cap presence + per-radio telemetry (freq, channel, RX/TX bytes). Faraday Mode state shown here.                                                                                                              |

## WiFi

| App              | What it does                                                                                                                                                                                                  |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **WiFi**         | AP scan + connect + auto-reconnect. Fn+Backspace forgets stored creds.                                                                                                                                        |
| **Pineapple**    | Tabbed: Dashboard (CPU/mem/temp/clients/SSIDs) → Recon (start scan, poll, completion) → Handshakes (count). Tab cycles tabs, Enter refreshes the current tab.                                                 |
| **Probes**       | Probe-request fingerprinting. Hops 1..13 every 250 ms. Records (SSID, client MAC) → count, up to 64 entries.                                                                                                  |
| **Handshake**    | Passive WPA capture. Streams every mgmt+data frame to `/handshakes/cap-<unix>.pcap`. Tab toggles 4-way ↔ PMKID-only mode (PMKID parser scans for the RSN PMKID KDE; pcap is parseable by `hashcat -m 22000`). |
| **Rogue AP**     | Tabbed: EvilTwin (Pineapple PineAP toggle) → Karma (Pineapple probe-responder) → Captive (local SoftAP + DNS hijack + form capture). Tab cycles modes, Fn+Enter enables, Backspace stops.                     |
| **Deauth**       | One app, two backends. `b` toggles Pineapple-driven (HTTP) ↔ S3-native (libnet override). Tab arms, Fn+Enter fires. Native path requires the override built in (default since v1.2).                          |
| **Beacon Flood** | Rotating fake APs from a configurable SSID list (8 defaults). Tab toggles, ^/v changes channel.                                                                                                               |
| **WPS Scan**     | Channel-hopping passive listener for beacons advertising the Microsoft WPS IE. APs with WPS=Yes are flagged.                                                                                                  |

## Bluetooth

| App           | What it does                                                                                                                                                                                |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **BLE**       | Passive scan of nearby BLE devices.                                                                                                                                                         |
| **BLE Spam**  | Cycles three known nuisance payloads (Apple iBeacon, Samsung Easy Setup, Google Fast Pair). Tab cycles rail (NimBLE / nRF24 / Both — Both only available with Hydra cap), Fn+Enter toggles. |
| **BLE Jam**   | Rapid empty-payload BLE adv (~100/s). Same Tab-rail picker as BLE Spam. 30s auto-shutoff failsafe.                                                                                          |
| **GATT**      | Connect to a BLE device by MAC. Walks services, drills into characteristics. Shows UUIDs + properties.                                                                                      |
| **IR Remote** | Universal IR blast.                                                                                                                                                                         |

## System

| App               | What it does                                                                                                                                                                |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Self-Test**     | 14-step programmatic bring-up: display, keyboard, SD, NVS, WiFi, BLE, IR, IMU, mic, speaker, clock, CC1101, nRF24, registry headroom. Press Enter, watch the verdict.       |
| **Hydra Status**  | (also reachable from Radio) Cap presence + Faraday Mode state.                                                                                                              |
| **bb-link Probe** | Three-phase chain check for the islandmagic/bb-link bridge: BT scan for "B.B. Link" → BLE connect → GATT walk for the Nordic UART service. `BridgeReady` if all three pass. |
| **Files**         | microSD browser. Enter on a file shows its content; Tab toggles hex view.                                                                                                   |
| **Sysinfo**       | Battery, free heap, uptime, IP, RSSI, wallclock, GPS fix status.                                                                                                            |
| **Settings**      | Brightness / Timezone / NTP / Theme / Faraday Mode / Boot SelfTest. ^/v cycles rows, </> changes the selected row.                                                          |
| **About**         | Version, build info.                                                                                                                                                        |

## NVS keys reference

Yui persists configuration in NVS namespace `yui`. Set via serial monitor or future Settings UI.

| Key           | Type | Purpose                                        |
| ------------- | ---- | ---------------------------------------------- |
| `brightness`  | int  | LCD brightness 0..100                          |
| `wifi.ssid`   | str  | Home WiFi SSID                                 |
| `wifi.pass`   | str  | Home WiFi passphrase                           |
| `clock.tz`    | str  | POSIX TZ string                                |
| `clock.ntp`   | str  | NTP server hostname                            |
| `ui.theme`    | str  | Palette id (`kohaku` / `ogon` / `asagi` / ...) |
| `sys.faraday` | int  | Faraday Mode flag (0/1)                        |
| `sys.lab_lat` | int  | Lab latitude × 1e6 (micro-degrees)             |
| `sys.lab_lon` | int  | Lab longitude × 1e6                            |
| `sys.boot_st` | int  | Run SelfTest on boot before launcher           |
| `radio.mac`   | str  | bb-link bridge BLE MAC for auto-connect        |
| `radio.pin`   | str  | BT-Classic PIN (default `0000`)                |
| `tx.call`     | str  | Your callsign for APRS TX                      |
| `tx.ssid`     | int  | APRS SSID 0..15                                |
| `pa.host`     | str  | Pineapple host IP                              |
| `pa.port`     | int  | Pineapple REST port (default 1471)             |
| `pa.user`     | str  | Pineapple username                             |
| `pa.pass`     | str  | Pineapple password                             |
| `pa.ssid`     | str  | Pineapple WiFi SSID                            |
| `pa.psk`      | str  | Pineapple WiFi passphrase                      |
| `cp.ssid`     | str  | Captive Portal advertised SSID                 |
| `cp.html`     | str  | Captive Portal HTML template (≤1024 bytes)     |

## Files Yui writes to SD

| Path                          | Content                                  |
| ----------------------------- | ---------------------------------------- |
| `/yui-tles.txt`               | SatTracker TLE database (3-line records) |
| `/yui-tracks/<unix>.gpx`      | GpsApp trackpoints when recording        |
| `/sub/<freq>_<ts>.sub`        | SubGhz captures (Flipper-compatible)     |
| `/handshakes/cap-<unix>.pcap` | WifiHandshake captures                   |
| `/captures.txt`               | Rogue AP (Captive mode) form submissions |
| `/rfchaos/<ts>.log`           | RfChaosApp event log (CSV)               |
| `/soak/<boot_ts>.csv`         | SoakLog heap/uptime trace                |

## Legal

Several apps transmit RF or interact with networks/devices around you. **Use only on RF spectrum, networks, and devices you own or have written authorization to test.** TX of WPA deauth, BLE spam, beacon flood, captive-portal phishing, and the like against parties who haven't consented is a federal crime in the US (CFAA / Wiretap Act / FCC Part 15) and equivalent elsewhere.

Yui surfaces "use only on YOUR network" hints in every app that does this. That's not legal advice, it's a reminder to a future-you. Combine the aggressive RF apps with **Faraday Mode** (Settings) so the strip is conditional on a controlled RF environment.

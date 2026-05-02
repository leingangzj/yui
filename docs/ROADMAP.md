# Roadmap

> Personal hobby project. One developer, no team, no funding. Updated
> to match what's actually shipping rather than what sounded good in a
> design doc.

## v0.1 — Foundation + utility apps (DONE, commit 0689297)

Shipped 2026-05-01. 21 apps across the bare ADV's hardware:

- HAL split (display, keyboard, clock, log, net, IMU, fs, IR, mic, speaker, storage)
- WiFi connect + auto-NTP + persistence
- TZ + NTP-server pickers
- Settings, Clock (TimeOfDay incl.), Sysinfo, About
- Notes, Todo, Files, Calc (RPN+trig+memory), Mic FFT, IR remote, IMU bubble level, KeyTest
- Pomodoro / Metronome / Tone / Snake / Life / Draw / Calendar
- Animated hinomaru-koi splash
- Categorized launcher (Flipper-style 6-category drill-in)

Definition of done: `pio test -e native` green, `pio run -e cardputer_adv` produces a flashable .bin.

## v0.2 — Tri-radio companion + Bruce-parity sweep (DONE, commit 8af44c0)

Shipped 2026-05-01. The actual identity of the device.

**Track A — Kenwood TH-D75 over BT-Classic SPP:**

- KenwoodApp (status / pair / refresh)
- AprsApp (RX station list)
- AprsMessageApp (TX via KISS)
- GpsApp (NMEA → GPX log)
- RemoteHeadApp (VFO / freq / mode editor)

**Track B — WiFi Pineapple over WiFi/SSH/REST:**

- PineappleApp (dashboard cards)
- PineappleReconApp (paged AP list)
- HandshakeBrowserApp (count captured handshakes)
- EvilTwinApp / KarmaApp / WifiDeauthApp

**Track C — Cardputer onboard 2.4 GHz:**

- WifiHandshakeApp (passive handshake capture to .pcap)
- WifiProbeApp (probe-request fingerprinting)
- WifiNativeDeauthApp (via patched libnet ieee80211_freedom_output)
- WifiBeaconFloodApp (rotating fake APs)
- CaptivePortalApp (SoftAP + DNS-redirect-all + HTML form capture)
- WpsScanApp (WPS-IE detection in beacons)
- BleSpamApp / BleJammerApp / BleGattApp

**Native v0.1 + v0.2:** ~40 apps. 343 native tests. Flash 1623 KB / 6.4 MB.

**Status:** all apps work end-to-end against native fakes. ESP32 backends for the network/radio HALs are stubs awaiting v0.3 hardware bring-up.

## v0.3 — SatTracker + ESP32 hardware bring-up (DONE in firmware, head 2d98a4a)

Two pieces, only two pieces:

### SatTracker (headline)

ISS + amateur-satellite tracker. TLE database on SD (CelesTrak refresh weekly), favorites with next-pass countdown, polar-plot live pass display, auto-GPS from TH-D75 NMEA, drift-free time (NTP at boot + GPS discipline), Doppler-correcting CAT control of the TH-D75. SGP4/SDP4 propagator vendored from a public-domain reference.

**Effort:** ~4–6 weekends.

### ESP32 hardware bring-up

Replace 8 stubs with real implementations:

- `Esp32RadioLink` — `BluetoothSerial` to TH-D75
- `Esp32Gnss` — peel NMEA off the same SPP socket
- `Esp32Http` — `HTTPClient`/`WiFiClient` for Pineapple REST
- `Esp32Pcap` — libpcap streamer to SD
- `Esp32WifiMonitor` — `esp_wifi_set_promiscuous_*` plus the patched-libnet `tx_raw` path for native deauth
- `Esp32BleAdvertiser` — NimBLE GAP advertise
- `Esp32WifiAp` — `WiFi.softAP()` + DNS server + HTTP captive portal
- `Esp32BleCentral` — NimBLE BLEClient + GATT walk

Plus the 14-step Phase 0 hardware bring-up checklist in `docs/HARDWARE_AUDIT.md`.

**Effort:** ~3–4 weekends with the device in hand.

## v1.0 — Ship (in progress)

- ✅ M5Burner-compatible single-binary release script (`tools/pack-release.sh`)
- ✅ README rewritten for v0.3 reality (tri-radio companion + Bruce parity + SatTracker)
- ✅ End-user install guide (`docs/INSTALL.md`)
- ✅ End-user reference (`docs/USING.md`) — every app + every NVS key + every SD path
- ⏳ Hardware bring-up — pre-staged: `tools/bringup-checklist.md` (print-friendly 14-step Phase 0) + bb-link bridge pairing per `docs/protocols/KENWOOD_THD75.md`
- ⏳ 24-hour soak test — pre-staged: `tools/soak-monitor.py --port /dev/ttyACM0 --hours 24`; pass criterion: zero panics, zero WDTs, ≤3 boot banners, no >25% heap drops
- ⏳ Demo video — pre-staged: `tools/demo-video-shotlist.md` (3-min cut, three-act script, capture-order plan, failure-mode reshoots)
- ⏳ Public mirror with issues open

**Effort remaining:** ~1-2 weekends with hardware in hand. All non-hardware prep (release script, image validation, checklist, soak monitor, shot list) is shipped — see `tools/`.

### Known v0.3 caveats that v1.0 may revisit

1. **Native deauth via patched libnet** — `Esp32WifiMonitor::tx_raw` calls
   `esp_wifi_80211_tx`, which Espressif blocks for deauth/disassoc. The
   patched-libnet workaround (vendored `libnet80211.a` + `-zmuldefs` linker
   flag) isn't shipped in v0.3. If you want real native deauth on hardware,
   add `board/lib_extra/` with the patched archive — Bruce / ESP32-Marauder
   document the technique. Yui's UI already gates this app behind
   Tab-arms-Fn+Enter-fires.

2. **bb-link bridge** — Track A apps (TH-D75 ham radio integration) need
   a separate ESP32 (original) board running [islandmagic/bb-link](https://github.com/islandmagic/bb-link)
   firmware. Yui scans for "B.B. Link" advertised name and connects via
   BLE to its GATT service. ~$5 of additional hardware, one-time setup.

3. **WiFi+BT-Classic coexistence is moot** because we don't use BT-Classic
   on the Cardputer side — but **WiFi STA + BLE central + WiFi monitor
   mode** all on the S3 simultaneously hasn't been validated. Phase 0
   stress test will reveal whether we need to serialize radio access by
   category.

4. **Pcap streaming throughput on SD** — SD writes can stall under load.
   `Esp32Pcap` flushes every 16 KB; if real captures show packet drops,
   ring-buffer in RAM first.

---

## What's explicitly OUT of scope

Decided 2026-05-01 — these are NOT v2.0, NOT "later," they're dropped:

- **CC1101** (sub-GHz remote replay / spectrum)
- **PN532** (13.56 MHz NFC R/W)
- **125 kHz RFID** (LF tag work)
- **iButton** (Dallas 1-Wire)
- **LoRa-1262 Cap** (LoRa / Meshtastic / GNSS-via-Cap)
- **BME680 / environmental sensors**
- **BadUSB / HID injection** (the Cardputer USB-C is device-mode-only — never works on this hardware regardless)

Yui's scope is the bare ADV + the radios already on Zac's desk (TH-D75, Pineapple, FT5DR, ID-50, UV-K5, USB monitor stick). Anything requiring an additional Cap or external module is not on the roadmap.

---

## "Done enough" definition for v1.0

> A friend with an ADV reads the README, flashes the M5Burner image,
> pairs it with their TH-D75 over Bluetooth, points it at an upcoming
> ISS pass, and works the bird. No phone, no laptop. They also use
> it as a hand-held remote for their Pineapple at a paid pentest gig.
> Both work first try.

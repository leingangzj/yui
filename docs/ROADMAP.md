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

## v1.0 — Ship (firmware shipped; hardware validation pending)

Shipped in the codebase:

- ✅ M5Burner-compatible single-binary release script (`tools/pack-release.sh`)
- ✅ Browser-based web flasher at <https://leingangzj.github.io/yui> (ESP Web Tools, served from GitHub Pages)
- ✅ End-user install guide (`docs/INSTALL.md`) — three flash paths, friction-ordered
- ✅ End-user reference (`docs/USING.md`) — every app, every NVS key, every SD path
- ✅ Release CI (`.github/workflows/release.yml`) — tag triggers test → build → pack → web-installer refresh → asset upload
- ✅ Public mirror at <https://github.com/leingangzj/yui> with Issues + Discussions enabled

Still pending — hardware-gated, will land in v1.0.x patch releases:

- ⏳ Phase 0 hardware bring-up — pre-staged in `tools/bringup-checklist.md` (14-step print-friendly checklist)
- ⏳ bb-link bridge pairing with the TH-D75 — protocol notes in `docs/protocols/KENWOOD_THD75.md`
- ⏳ 24-hour soak test — pre-staged in `tools/soak-monitor.py`; pass criterion: zero panics, zero WDTs, ≤3 boot banners, no >25% heap drops
- ⏳ M5Burner submission — manual form at M5; checklist in `docs/RELEASING.md`

### Known caveats v1.0.x may revisit

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

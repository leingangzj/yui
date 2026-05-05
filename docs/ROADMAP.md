# Roadmap

Personal hobby project. One developer, no team, no funding. Updated to match what actually shipped, not what sounded good in a design doc.

## v0.1 — Foundation (shipped 2026-05-01, `0689297`)

21 apps across the bare ADV's hardware. HAL split (display, keyboard, clock, log, net, IMU, fs, IR, mic, speaker, storage). WiFi connect + auto-NTP + persistence. Categorized launcher (Flipper-style six-category drill-in). Animated hinomaru-koi splash.

## v0.2 — Tri-radio companion (shipped 2026-05-01, `8af44c0`)

The actual identity of the device. ~40 apps total.

- **Kenwood TH-D75 over BT-Classic SPP:** Kenwood, APRS RX/TX, GPS log, RemoteHead VFO control
- **WiFi Pineapple Mark VII over REST:** dashboard, recon, evil-twin, karma, deauth, handshake browser
- **Cardputer onboard 2.4 GHz:** WPA handshake capture, probe-request fingerprinting, native deauth, beacon flood, captive portal, WPS scan, BLE spam/jam/GATT

343 native tests. Flash 1623 KB / 6.4 MB. ESP32 backends for the network/radio HALs are stubs awaiting v0.3.

## v0.3 — SatTracker + ESP32 bring-up (shipped, `2d98a4a`)

ISS + amateur-satellite tracker: TLE database on SD with weekly CelesTrak refresh, polar-plot live pass display, auto-GPS from TH-D75 NMEA, drift-free time, Doppler-correcting CAT control. SGP4/SDP4 propagator vendored from a public-domain reference.

Replaced 8 stubs with real ESP32 implementations: BluetoothSerial radio link, NMEA parsing, HTTPClient for Pineapple, libpcap streaming, promiscuous-mode WiFi RX, NimBLE GAP advertise + central, SoftAP + DNS server.

## v1.0 — Public release (shipped 2026-05-02, tag `v1.0`)

- M5Burner-compatible single-binary release script (`tools/pack-release.sh`)
- Browser web flasher at <https://leingangzj.github.io/yui> (ESP Web Tools, GitHub Pages)
- End-user install + reference guides (`docs/INSTALL.md`, `docs/USING.md`)
- Tag-triggered release CI (test → build → pack → web-installer refresh → asset upload)
- Public mirror at <https://github.com/leingangzj/yui> with Issues + Discussions

Hardware-gated, will land in v1.0.x patch releases:

- Phase 0 hardware bring-up (now superseded by `SelfTestApp`)
- bb-link bridge pairing with the TH-D75
- 24-hour soak test
- M5Burner submission

## v1.1 — Hydra RF cap + utility prune (shipped 2026-05-04, tag `v1.1.0`)

Full Hydra RF cap support across nine apps; pruned 14 utility/game apps to focus the device on its RF/security identity.

- HAL: `IRadioCC1101`, `INrf24`, `HydraSpiBus`, Flipper-compatible `.sub` reader/writer, boot-time cap probe
- Apps: SubGhz Scan / Capture / Replay / Brute / Jammer; Nrf24 Scan / Jammer; Mousejack; RollJam; HydraStatus
- UI: `Chrome::cap_missing_dialog` shared across apps; `H+` / `H~` / `H-` cap badge in launcher status strip
- Pruned: Notes, Todo, Calendar, Pomodoro, Metronome, Tone, Mic, Imu, KeyTest, Snake, Life, Draw, TvBGone

`AppRegistry::kMaxApps` raised 40 → 64. 362/362 native tests passing.

## v1.2 — Lab Mode (shipped 2026-05-05, tag `v1.2.0`)

The Faraday-room sweep. 38 commits since v1.1.0, 387 native tests, zero build warnings, launcher 42 → 32 entries.

- **Native deauth actually works** — 12-line C override of Espressif's `ieee80211_freedom_inside_cb` compiled into the firmware via `build_src_filter`. Verified live in `firmware.elf` (symbol size 0x07 vs. stock 0x2e). Source committed; no vendored binary.
- **FaradayMode** flag persisted to NVS, with optional GPS lab-location guard. Aggressive RF apps strip their politeness gates and uncap duty cycles when on. Red `LAB` badge in the launcher status strip.
- **RfChaosApp** — random-target RF stress orchestrator across BLE / nRF24 / CC1101 / WiFi monitor. Refuses to arm without FaradayMode. Logs to `/rfchaos/<ts>.log`.
- **Dual-rail BLE** — `IBleRawTx` HAL lets `BleSpamApp` and `BleJammerApp` drive S3 NimBLE and Hydra nRF24 simultaneously, channel-divided.
- **Hydra refinements:**
  - nRF24 RPD-bit promiscuous read-back (Mousejack catches devices the user never paired)
  - CC1101 edge-toggle TX migrated to `esp_timer_get_time()` + worst-edge jitter trace
  - CC1101 packet/preamble IRQ HAL → SubGhzJammer Protocol/Reactive modes
  - SubGhzScan smart dwell (N samples/bin) + auto-tune
- **PMKID-only mode** for `WifiHandshakeApp` (faster than the 4-way path).
- **Bench-prep apps:** SelfTestApp (replaces paper bring-up checklist), BbLinkProbeApp (bridge diagnostic), SoakLog (60s heap/uptime CSV).
- **Audit cleanup:** zero `-Wformat-truncation` / `-Wrestrict` warnings, footer keybinding convention unified, four BLE/WiFi apps migrated to `draw_text_styled`, Title Case subtitles.
- **Final consolidation (4.13.7):** dropped `KoiGotchiApp`, `CalculatorApp`, `ClockApp`. Launcher 35 → 32.

## v1.3 — On-device LAN/Service Recon (planned)

Cardputer ADV + Hydra cap on its own can do the RF half of a homelab pen-test. The LAN half — port scanning, service fingerprinting, credential spraying, MITM — is still missing. v1.3 fills the gap, all on-device, no host scripts, no R620/T5820 dependence.

| App            | What                                             | Notes                                                                       |
| -------------- | ------------------------------------------------ | --------------------------------------------------------------------------- |
| `LanScanApp`   | LWIP TCP connect scan + banner grab              | top-20 / top-100 / custom port profile, ~200 ports/sec                      |
| `MdnsSsdpApp`  | mDNS + SSDP passive discovery + one M-SEARCH     | <8 KB heap, fully passive after the probe                                   |
| `CredSprayApp` | HTTP basic / form-login / SSH (libssh2) / Telnet | refuses to run unless FaradayMode on, hits never echo to display by default |
| `BadUsbApp`    | USB-OTG HID with DuckyScript subset              | Cardputer ADV's S3 has native USB-OTG; no extra hardware                    |
| `HttpReconApp` | Web service fingerprint over ~30 signatures      | matches Proxmox/Gitea/Jellyfin/Plex/\*arr/Ollama/Zabbix/etc                 |

Estimated ~12 evenings, +11 apps (61 total, fits `kMaxApps=64`), +220 native tests, +180 KB flash (libssh2 dominates).

## What's left (hardware-only, can't be done from a clean checkout)

- Vendor a known-good patched `libnet80211.a` if you prefer a binary to the source-stub override. v1.2's source path works fine; this is optional.
- Phase 0 hardware bring-up — `SelfTestApp` now does the 14-step probe in software; operator runs once and confirms.
- bb-link bridge pairing with a real TH-D75 — `BbLinkProbeApp` validates from the Cardputer side.
- 24-hour soak test — `SoakLog` writes the CSV; `tools/soak-monitor.py --read-soak-csv` analyzes it.
- M5Burner submission — manual web form per `docs/RELEASING.md`.

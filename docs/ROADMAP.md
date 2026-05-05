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

## v1.0+ — Hydra RF cap support (DONE, 2026-05-04)

Shipped in the codebase across nine commits the same night:

- **Phase 1 (`aea28a9`)** — Styled-font system in IDisplay
  (Title/Body/Caption/Mono via real M5GFX bundled fonts) +
  Chrome widget kit (header/footer/list_row/badge/empty/stat)
  refit for the new fonts; new Chrome widgets for Phase 4
  (`scrollbar`, `var_item_row`, `dialog`).

- **Phase 2 (`d69c07e`)** — Bitmap icon pipeline. 32 Japanese-
  themed icons sliced from `menu1.png` + `menu2.png` via
  `tools/extract_icons.py` (auto 4×4 grid detect), embedded by
  `tools/gen_assets.py::gen_icons` into
  `include/yui/assets/icons.hpp`. Semantic alias namespace
  (`kRadio()`, `kWiFi()`, etc.) so renaming a slot edits one file.

- **Phase 3 (`276aecc`)** — Hydra HAL. `ICc1101` + `INrf24`
  abstract interfaces with native fakes for tests; ESP32 impls
  using RadioLib over a shared `HydraSpiBus` singleton; pin map
  in `yui::pins` (CS=13/io0=5 for CC1101, CS=6/io0=4 for nRF24);
  Flipper-compatible `.sub` reader/writer; boot-time cap probe
  in `main.cpp`.

- **Phase 4 (`394b323`)** — 9 RF apps wired into the launcher,
  each with cap-detect scaffolding: `SubGhzScan`,
  `SubGhzCapture`, `SubGhzReplay`, `SubGhzBrute`,
  `SubGhzJammer`, `Nrf24Scan`, `Nrf24Jammer`, `Mousejack`,
  `RollJam`. Three live, six functional stubs.

- **Phase 4.5 (`429eb02`)** — Replay file picker + load + transmit;
  Brute 24-bit dictionary walker; Jammer Noise + Sweep modes.

- **Phase 4.6 (`70b25d9`)** — Mousejack scan/inject and RollJam
  state machine drive real radio operations.

- **Phase 4.7 (`bc31aa9`)** — `docs/HYDRA.md` operator guide;
  `HydraStatusApp` diagnostic.

- **Phase 4.8 (`75b0a96`)** — `Chrome::cap_missing_dialog`
  refactor — 9 apps share one helper now.

- **Phase 4.9 (`054adeb`)** — Hydra cap badge in launcher status
  strip (`H+` / `H~` / `H-`).

Total: ~50 apps, 423 native tests, RAM 56% / Flash 66%.

### Phase 4.x+ — hardware-gated

- Mousejack promiscuous-mode nRF24 sniff (RadioLib doesn't expose
  the chip's RPD bit; needs direct register write).
- Edge-toggle async sub-GHz TX for Flipper-perfect rolling-code
  replay (needs GDO0 toggling in tight loops).
- Preamble-detect IRQs to unlock SubGhzJammer Protocol/Reactive
  modes and tighter RollJam timing.
- SubGhzScan smarter dwell (currently 1 sample/tick).

## v1.1 — Hydra RF cap + utility prune (SHIPPED, 2026-05-04)

Tagged as `v1.1.0`. Captures everything in `v1.0+ — Hydra RF
cap support` above plus the `refactor: prune utility/game apps`
sweep that followed.

- Hydra Phase 1 → 4.11 — 9 RF apps wired to CC1101 + nRF24
  via `IRadioCC1101` / `INrf24` HALs, `.sub` file format,
  Replay/Brute/Jammer/Mousejack/RollJam fully driven.
- `Chrome::cap_missing_dialog` shared helper (Phase 4.8).
- Hydra cap badge in launcher status strip (Phase 4.9).
- `HydraStatusApp` diagnostic + `docs/HYDRA.md` operator guide.
- Pruned 14 utility/game apps (Notes, Todo, Calendar, Pomodoro,
  Metronome, Tone, Mic, Imu, KeyTest, Snake, Life, Draw,
  TvBGone) + Snake/Life engines + Date util to focus the device
  on its RF/security identity. `AppRegistry::kMaxApps` 40 → 64.

**Native tests:** 362/362 passing. **Footprint:** flash trim
~3000 LOC removed; binary still well under 8 MB.

## Phase 4.13 — App consolidation (PLANNED, lands as v1.1.1)

Audit of the 42 registered apps surfaced 8 launcher slots
where two-or-three apps share an operator workflow. Consolidate
into single apps with mode toggles. Pure refactor — no new
capability, no test loss (tests follow the merged apps), better
operator UX.

**Result:** 42 → 34 launcher entries.

| Step   | Today                                                        | Merged into                                                                  | Effort |
| ------ | ------------------------------------------------------------ | ---------------------------------------------------------------------------- | ------ |
| 4.13.1 | `WifiDeauthApp` (Pineapple) + `WifiNativeDeauthApp` (S3)     | One `DeauthApp` with **Backend: Pineapple / Native** toggle                  | 1 hr   |
| 4.13.2 | `SubGhzCaptureApp` + `SubGhzReplayApp`                       | One `SubGhzCaptureReplayApp` with **Read / Saved** modes (Flipper-style UX)  | 2 hr   |
| 4.13.3 | `PineappleApp` + `PineappleReconApp` + `HandshakeBrowserApp` | One `PineappleApp` with **Dashboard / Recon / Handshakes** tabs              | 2 hr   |
| 4.13.4 | `EvilTwinApp` + `KarmaApp` + `CaptivePortalApp`              | One `RogueApApp` with three modes (clone-AP / probe-answer / SoftAP-capture) | 2 hr   |
| 4.13.5 | `Nrf24ScanApp` (standalone)                                  | Subsumed into `MousejackApp` as a **Generic Scan** mode                      | 1 hr   |
| 4.13.6 | `ThemeApp` (top-level)                                       | Folded into `SettingsApp` as a submenu                                       | 1 hr   |

**Total effort:** ~2 evenings.

### Explicitly NOT consolidated

- **Sub-GHz family** (Scan / Brute / Jammer + merged CaptureReplay)
  — different mental models; survey vs. attack vs. workflow.
- **BLE family** (Spam / Jammer / GATT / Scan) — distinct operations.
- **Ham radio suite** (Kenwood / APRS / GPS / RemoteHead / AprsMessage)
  — already coherent, each has one job.
- **A "MegaScanApp"** that unifies Sub-GHz + nRF24 + WiFi + BLE
  scans — Bruce/Marauder do this and it's awful to navigate.
  Per-band UX is right for this category.

### 4.13.7 — RESOLVED — `KoiGotchiApp` / `CalculatorApp` / `ClockApp` cut

All three deferred candidates removed. Final launcher count after
the consolidation sweep + RfChaosApp registration + this cut:
**32 entries** (42 → 32, 10 slots recovered, all off-mission utility
removed).

## v1.2 — Lab Mode (PLANNED)

Operator works exclusively in a Faraday-shielded lab. Goal:
remove the politeness gates that exist for external-RF-impact
reasons, and add the apps that only make sense when nothing
outside the room can hear you. Everything in this phase runs
on Cardputer ADV + Hydra cap alone. No host-side dependencies.

### Phase 5.0 — Faraday Mode flag

Single NVS key `system.faraday_mode = true` that:

- Strips the Tab-arm-Fn+Enter gate from `WifiNativeDeauthApp`,
  `BleJammerApp`, `SubGhzJammerApp`, `WifiBeaconFloodApp`,
  `MousejackApp` (inject path), `SubGhzBruteApp`.
- Removes duty-cycle caps in jammer apps (currently capped at
  ~30% to not burn the front-end and not own the band).
- Surfaces a red `LAB` badge in the launcher status strip,
  next to the existing `H+/H~/H-` Hydra cap badge.
- Refuses to flip ON unless either (a) GPS fix is missing OR
  (b) GPS fix matches a stored "lab location" within 50 m
  (NVS keys `system.lab_lat` / `system.lab_lon`). Belt-and-
  suspenders against forgetting to flip back outdoors.
- Persists to NVS, survives reboot, shown in `SettingsApp` and
  `HydraStatusApp`.

**Effort:** ~½ evening. **Tests:** ~6 native cases (gate
strip, GPS guard, persistence, badge render).

### Phase 5.1 — `WifiNativeDeauthApp` ship

Stop holding back the patched-libnet workaround. Drop the
vendored `libnet80211.a` into `board/lib_extra/` with
`-zmuldefs` linker flag (technique documented by Bruce /
ESP32-Marauder). App already exists; this is a build-system
change to actually let it transmit deauth/disassoc frames.

**Effort:** ~½ evening. Hardware-validation gated until first
soak.

### Phase 5.2 — Dual-rail BLE (Phase 4.12 spec, formalized)

Drive S3 NimBLE radio AND Hydra nRF24 as independent BLE-band
TX engines simultaneously.

**HAL:** new `IBleRawTx` interface, two impls (`NimBleRawTx`,
`Nrf24BleRawTx`). Channel-selectable per-rail (37/38/39).

**Apps touched:** `BleSpamApp`, `BleJammerApp` gain a "Rail:
NimBLE / nRF24 / Both" Settings line and a 3-row channel
matrix. Defaults split channels to avoid self-collision
(NimBLE→37, nRF24→38+39).

**HydraStatusApp:** new line `BLE Dual-Rail: ARMED / IDLE /
CAP MISSING`.

**Tests:** ~10 native cases — channel split, rail toggle
across suspend/resume, BLE address whitening per Core spec
§6.B.3, self-collision detector.

**Effort:** ~1 evening. **Risk:** WiFi STA + BLE + nRF24
three-radio coex on S3 unvalidated; soak test will tell.

### Phase 5.3 — `RfChaosApp` (lab-only)

Randomly cycles jammer + deauth + BLE spam + beacon flood for
N minutes against the _operator's own_ WiFi/BLE infrastructure
in the Faraday room. Logs to SD which RSSI floors collapsed,
which channels recovered, and timing of TX bursts.

UI: duration picker (1/5/15/60 min), intensity slider (1–10),
target list (which apps to include), START/STOP. Refuses to
arm unless `system.faraday_mode == true`.

Output file: `/sd/rfchaos/<timestamp>.log` — CSV of (ms,
event, channel, rssi). Reviewable on-device via `FilesApp` or
off-device after SD pull.

**Effort:** ~1 evening. **Tests:** ~8 native cases — schedule
sanity, faraday-gate refusal, log writer, app composition.

### Phase 5.4 — PMKID capture mode for `WifiHandshakeApp`

Adds a "PMKID-only" mode alongside the existing 4-way
handshake mode. Sends a single EAPOL request to a target BSSID
and writes the PMKID-bearing frame to a separate `.pcap` if
the AP responds (most modern APs, including Eero, do).
Faster-to-result than waiting for a natural reassoc.

Implementation: existing `Esp32WifiMonitor` already has the
TX path; this is one new EAPOL builder and a parser to
extract PMKID from the M1 reply.

**Effort:** ~1 evening. **Tests:** ~6 native cases — EAPOL
construction, PMKID extraction, pcap framing.

### Phase 5.5 — Aggressive `MousejackApp` + `SubGhzBruteApp`

unlock

In Faraday Mode only:

- Mousejack inject: removes the conservative inter-packet
  spacing; runs at the chip's max retry rate (~500 pkt/s
  vs. current ~50). See if Logitech Unifying receivers
  actually deauth under load.
- SubGhzBrute: full duty cycle on 24-bit OOK keyspace
  (current cap ~30%). Finishes `2^24` keys at 5 ms/key in
  ~23 hours; lab session usually only needs a fraction.

Both already coded; this phase is gate-removal + Settings UI
exposing the unlocked rate sliders.

**Effort:** ~½ evening. **Tests:** existing apps already
covered; add ~4 cases for the rate-slider clamp logic.

## v1.3 — On-device LAN/Service Recon (PLANNED)

The actually-useful homelab pentest layer. Cardputer ADV's
WiFi STA joins the lab SSID and runs everything on-device —
no host scripts, no R620/T5820 dependence. Hydra cap idle for
this phase (RF unused). Output goes to SD + on-device UI.

### Phase 6.0 — `LanScanApp` (port + service scanner)

Nmap-lite. Operator picks a CIDR (default = own subnet from
DHCP lease), picks a port profile (top-20 / top-100 /
custom), starts scan. Async TCP connect scan via LWIP socket
API on the S3 — no raw sockets needed.

For each open port, attempts a one-shot service banner read
(50 ms timeout). HTTP gets a `GET / HTTP/1.0\r\n\r\n` and
captures `Server:` + first <title>. SSH/FTP/SMTP grab the
greeting line. Everything else is recorded as
`open/<size>-byte-banner`.

Result table on screen: IP / port / service-guess / banner
preview. Persisted to `/sd/lan/<timestamp>.json`. Sortable;
selectable rows pivot into Phase 6.2 `CredSprayApp`.

Resource budget: 256 in-flight sockets, ~30 KB heap, scan
rate ~200 ports/sec on a /24 = full top-20 sweep in ~25 s.

**Effort:** ~2 evenings. **Tests:** ~14 native cases — CIDR
parser, port-profile loader, banner extractor, JSON writer,
result-table sort.

### Phase 6.1 — `MdnsSsdpApp` (passive discovery)

Listens on 224.0.0.251:5353 (mDNS) + 239.255.255.250:1900
(SSDP) for 30 s after entering. Builds a service inventory
table: hostname / IP / service-type / TXT record summary.
Captures Plex, Jellyfin, Chromecast, AirPlay, Sonos, Eero,
Proxmox, \*arr, every Bonjour-aware thing on the lab LAN.

Also emits one SSDP M-SEARCH (`ssdp:all`) at start to provoke
quick replies.

Result persisted to `/sd/mdns/<timestamp>.json`. Selectable
rows pivot into `LanScanApp` (auto-fills target IP) or open a
URL preview if HTTP service.

Resource budget: <8 KB heap, fully passive after the single
M-SEARCH.

**Effort:** ~1 evening. **Tests:** ~10 native cases — mDNS
parser, SSDP parser, dedup, TXT record decode.

### Phase 6.2 — `CredSprayApp` (default-creds checker)

Takes a target list (manual entry, or auto-imported from
`LanScanApp` results) and a credential list (built-in
defaults + user-editable `/sd/creds/wordlist.txt`).

Protocol modules — one connection attempt per (target, creds)
tuple, conservative timeout, single-threaded:

- HTTP Basic / Digest auth
- HTTP form-login (configurable POST body template per host
  profile, e.g. Proxmox, Gitea, Jellyfin, Sonarr)
- SSH password auth (libssh2 vendored, ~80 KB flash cost)
- Telnet (banner + login prompt detection)

Built-in defaults wordlist (~50 entries): admin/admin,
root/root, admin/password, plus the famous vendor defaults
(ubnt/ubnt, pi/raspberry, etc.). Operator's lab-specific
list goes in SD.

Hits logged to `/sd/creds/hits-<timestamp>.json` with
host/port/protocol/creds. **Never echoes successful creds to
display by default** — Settings toggle to opt-in. Refuses to
run unless `system.faraday_mode == true` OR a one-time
"acknowledged scope" prompt (Tab-arm-Fn+Enter) is satisfied
per session.

Resource budget: ~120 KB flash with libssh2, ~40 KB heap
during a run, ~2 attempts/sec sustainable.

**Effort:** ~3 evenings (libssh2 vendoring is the bulk).
**Tests:** ~16 native cases — auth-module dispatch, wordlist
parser, hit serializer, scope-gate refusal.

### Phase 6.3 — `BadUsbApp` (USB-OTG HID)

Cardputer ADV's S3 has native USB-OTG; no extra hardware.
App presents Cardputer as a USB HID keyboard (composite
device with the existing CDC retained for serial).

Payload format: a stripped DuckyScript subset (`STRING`,
`DELAY`, `ENTER`, `GUI`, `CTRL+ALT+DEL`, `REM`). Payloads
live on SD as `/sd/badusb/*.duck`. App lists them, operator
picks one, gets a confirmation screen, presses Fn+Enter to
fire on next USB host enumeration.

Use case: plug Cardputer into iDRAC USB / Proxmox console /
any KVM, type a canned command sequence (e.g. drop an SSH
key, dump `/etc/shadow` hash to a USB-mounted SD partition
the operator owns).

Layout: US ANSI default, settable in Settings (DE/FR/JP
included for completeness).

**Effort:** ~1 evening. **Tests:** ~8 native cases —
DuckyScript parser, keymap translation, payload validator.

### Phase 6.4 — `HttpReconApp` (web service fingerprint)

Targets one URL at a time (autofills from `LanScanApp` /
`MdnsSsdpApp` selection). Fetches `/`, `/robots.txt`,
`/.well-known/security.txt`, `/sitemap.xml`, common admin
paths (`/admin`, `/login`, `/api`, `/swagger`, `/graphql`).
Records status, server header, content-type, response size,
first 256 bytes.

Heuristic fingerprinter: matches against ~30 built-in
signatures (Proxmox, Gitea, GitLab, Jellyfin, Plex, Sonarr,
Radarr, Prowlarr, Bazarr, Tautulli, qBittorrent, SABnzbd,
Ollama, Zabbix, Linkding, Memos, ByteStash, Paperless-ngx,
Radicale, Eero admin, generic nginx/Apache/Caddy).

Output: per-target report on screen + JSON to
`/sd/httprecon/<timestamp>-<host>.json`.

**Effort:** ~1 evening. **Tests:** ~10 native cases — path
list iteration, signature matcher, JSON writer.

## v1.2 + v1.3 summary

| Phase | What                                  | Effort | Notes                     |
| ----- | ------------------------------------- | ------ | ------------------------- |
| 5.0   | Faraday Mode NVS flag                 | ½ ev   | Gate-stripper + GPS guard |
| 5.1   | Ship patched libnet for native deauth | ½ ev   | Build-system only         |
| 5.2   | Dual-rail BLE (NimBLE + nRF24)        | 1 ev   | Three-radio coex risk     |
| 5.3   | `RfChaosApp` lab stress test          | 1 ev   | Faraday-gated             |
| 5.4   | PMKID capture mode                    | 1 ev   | Faster than 4-way         |
| 5.5   | Aggressive Mousejack + Brute unlock   | ½ ev   | Gate-removal + sliders    |
| 6.0   | `LanScanApp`                          | 2 ev   | LWIP TCP connect scan     |
| 6.1   | `MdnsSsdpApp`                         | 1 ev   | Passive + 1 M-SEARCH      |
| 6.2   | `CredSprayApp`                        | 3 ev   | libssh2 vendoring is bulk |
| 6.3   | `BadUsbApp`                           | 1 ev   | USB-OTG HID, no extra HW  |
| 6.4   | `HttpReconApp`                        | 1 ev   | ~30 service signatures    |

**Total:** ~12 evenings of software work. Adds ~11 apps
(rough new total ~61, headroom in `kMaxApps=64`). Estimated
+220 native tests. Estimated flash budget +180 KB (libssh2
dominates).

**Hardware required beyond what's in hand:** none. All v1.2

- v1.3 work runs on Cardputer ADV + Hydra cap alone.

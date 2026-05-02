# Yui

> 結い — _Okinawan: cooperative community work, the bond that ties people together._

A pocket-sized **comms + pentest companion** for the M5Stack Cardputer ADV.

Yui pairs the Cardputer with the radios and tools you already own and turns
them into a single hand-held console — a Kenwood TH-D75 ham radio (via a
small bb-link bridge), a WiFi Pineapple, and the Cardputer's onboard 2.4 GHz
radio for native WiFi/BLE work — plus a satellite tracker that drives the
TH-D75's frequency for Doppler shift during a pass.

```
   ┌────────────────────────────────────┐
   │   Yui                              │
   │ ┌─────────┐ ┌─────────┐ ┌────────┐ │
   │ │ Radio   │ │ WiFi    │ │ Bluetooth│
   │ │ APRS    │ │ Probes  │ │ Spam   │ │
   │ │ GPS     │ │ Handshake│ │ GATT   │ │
   │ │ SatTrack│ │ Pineapple│ │ Jam    │ │
   │ └─────────┘ └─────────┘ └────────┘ │
   │ ┌─────────┐ ┌─────────┐ ┌────────┐ │
   │ │ Tools   │ │ System  │ │ Fun    │ │
   │ └─────────┘ └─────────┘ └────────┘ │
   └────────────────────────────────────┘
        ↕ BLE       ↕ WiFi STA      ↕ direct
     bb-link     WiFi Pineapple     onboard
        ↕ BT-Classic
    Kenwood TH-D75
```

## Status

🛰️ **v0.3 — feature-complete; awaiting hardware bring-up.** Both `pio run -e
cardputer_adv` and `pio test -e native` are green. ~40 apps across 6
categories. ~360 native unit tests. Real-hardware validation pending.

## Headline features

- **APRS station tracker** with TX/RX over the TH-D75
- **GPS logger** (GPX export) — uses the radio's onboard GNSS
- **SatTracker** — TLE database, next-pass predictor, polar-plot live view,
  Doppler-correcting CAT control of the TH-D75
- **WiFi Pineapple companion** — recon, Evil Twin / Karma, deauth, handshake
  browser, all driven from the Cardputer keyboard
- **Native WiFi tools** — probe-request fingerprinting, WPA handshake capture
  (passive), beacon flood, WPS scan, captive portal phishing with credential
  capture, native deauth (when patched-libnet is provided)
- **BLE tools** — scan, GATT enumeration, advertisement spam, jammer
- **TV-B-Gone** — multi-brand IR power-off blast
- Plus the v0.1 utility set: notes, files, calc (RPN+trig+memory), mic FFT,
  IR remote, calendar, todos, snake, life, tone/metronome, etc.

## What you'll need

| Required                  | Why                                                      |
| ------------------------- | -------------------------------------------------------- |
| **M5Stack Cardputer ADV** | The host. ESP32-S3FN8, 8 MB flash.                       |
| nothing else              | The bare ADV runs Yui + all native WiFi/BLE/IR features. |

| Optional                                                                                            | Unlocks                                                           |
| --------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------- |
| **Kenwood TH-D75 + bb-link bridge** ([islandmagic/bb-link](https://github.com/islandmagic/bb-link)) | APRS, GPS log, SatTracker CAT control, RemoteHead VFO/mode editor |
| **Hak5 WiFi Pineapple Mark VII**                                                                    | Pro WiFi-pentest tier — driven over the Pineapple's REST API      |

The ESP32-S3 in the Cardputer is BLE-only, so the TH-D75 (which is BT-Classic
SPP) needs the bb-link bridge — a separate ESP32 (original) board running
the open-source bb-link firmware that translates BLE↔BT-Classic. ~$5 board,
one-time pairing setup. See `docs/protocols/KENWOOD_THD75.md` for the
architecture.

## Quick start (developers)

```bash
# Run host-side tests (no hardware required)
pio test -e native

# Build firmware for the Cardputer ADV
pio run -e cardputer_adv

# Flash over USB
pio run -e cardputer_adv -t upload -t monitor
```

End users will be able to flash a prebuilt M5Burner image — that's the v1.0
deliverable; until then it's `pio` from source.

## Install (end users — coming with v1.0)

1. Download M5Burner.
2. Search for "Yui" in the Cardputer ADV section, click Burn.
3. After the device reboots, open WiFi from the Yui launcher and connect to
   your home AP. Settings → Timezone for your zone.
4. (Optional) Pair the bb-link bridge to your TH-D75 once via the bb-link
   companion app, then in Yui open Radio → Kenwood and Tab to connect.
5. (Optional) Configure your Pineapple host/credentials in NVS via the
   serial monitor. Open WiFi → Pineapple to verify.

## Architecture

HAL-first C++17 with strict separation between platform-agnostic logic and
ESP32-specific backends. ~360 native unit tests run on the host machine
against `Fake*` HALs; real-hardware testing is layered on top, not under,
the test pyramid.

See `docs/ARCHITECTURE.md` and `docs/HARDWARE_AUDIT.md`.

## Repo layout

```
docs/             HARDWARE.md, ARCHITECTURE.md, ROADMAP.md, HARDWARE_AUDIT.md,
                  protocols/{KENWOOD_THD75, AX25_APRS, PINEAPPLE_API,
                              ESP_WIFI_AND_PCAP}.md
include/yui/
   app/           ~40 app classes (RadioCategory, WiFiCategory, etc.)
   hal/           IRadioLink, IGnss, IHttp, IPcap, IWifiMonitor, IBleAdvertiser,
                  IBleCentral, IWifiAp, plus the v0.1 set (IDisplay, IKeyboard,
                  IClock, ILog, INet, IImu, IFs, IIr, IMic, ISpeaker, IStorage)
   proto/         AX.25 + APRS + KISS + Dot11 + Pcap + Pineapple decoders
   sat/           Tle, Vec3, Time, Propagator, Topo, Pass — all the math
                  the SatTracker needs
src/hal/native/   Fake* test backends (no Arduino/ESP-IDF deps)
src/hal/esp32/    Esp32* real backends — BluetoothSerial→bb-link via BLE,
                  HTTPClient, esp_wifi_set_promiscuous_*, NimBLE GATT, etc.
test/test_native/ ~360 host-side tests (one giant file for now)
platformio.ini    `native` and `cardputer_adv` envs
```

## License

MIT — see [LICENSE](LICENSE). Keep the copyright notice and do whatever you
want with it.

## Prior art

Yui takes ideas (not code) from [Bruce](https://github.com/BruceDevices/firmware)
(AGPL) and [NEMO](https://github.com/n0xa/m5stick-nemo) (GPL). Yui is MIT and
written fresh. The Bruce-parity sweep app set was built as clean-room work
against the same documented protocols Bruce uses, never copying their code.

## Legal

Several Yui apps transmit RF or send WiFi management frames that interact
with networks/devices around you. **Use only on RF spectrum / networks /
devices you own or have written authorization to test.** TX of WPA deauth,
BLE-spam, beacon flood, captive-portal phishing, and the like against
parties who have not consented is a federal crime in the US (CFAA / Wiretap
Act / FCC Part 15) and equivalent elsewhere. The author and contributors
take no responsibility for misuse.

## Roadmap

See `docs/ROADMAP.md`. Short version: v0.1 done, v0.2 done, v0.3 done
(awaiting hardware bring-up); v1.0 is the polish + ship pass.

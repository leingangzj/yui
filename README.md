# Yui

> 結い — _Okinawan: cooperative community work, the bond that ties people together._

Firmware for the M5Stack Cardputer ADV. Pen-test workbench in your pocket: WiFi, BLE, sub-GHz, 2.4 GHz, IR, and ham-radio control from one device with a real keyboard.

Built for people who already own a [Kenwood TH-D75][thd75], a [WiFi Pineapple][pa], and an opinion about how a hand-held console should work.

[thd75]: https://www.kenwood.com/usa/com/amateur/th-d75a/
[pa]: https://shop.hak5.org/products/wifi-pineapple

## Install

**Web flasher.** Open [leingangzj.github.io/yui][flasher] in Chrome, Edge, or Brave. Plug the device into a USB-C data port, click Install, pick the serial port. Done in about 90 seconds.

[flasher]: https://leingangzj.github.io/yui

**esptool.** Grab `yui-vX.Y-cardputer_adv.bin` from the [latest release][rel], then:

```bash
python3 -m esptool --chip esp32s3 -p /dev/ttyACM0 -b 921600 \
  write_flash --flash_mode qio --flash_freq 80m --flash_size 8MB \
  0x0 yui-v1.2.0-cardputer_adv.bin
```

[rel]: https://github.com/leingangzj/yui/releases/latest

**From source.**

```bash
pio test -e native                              # host tests, no hardware
pio run -e cardputer_adv                        # build firmware
pio run -e cardputer_adv -t upload -t monitor   # flash over USB-C
```

Full instructions and troubleshooting in [`docs/INSTALL.md`](docs/INSTALL.md).

## Features

<details>
<summary><b>WiFi</b></summary>

- Connect / disconnect, AP scan with RSSI, NTP sync
- Probe-request fingerprinting (passive)
- Passive WPA handshake capture → `.pcap` on SD
- PMKID-only mode (faster than waiting on a 4-way)
- Beacon flood (rotating fake APs)
- Native deauth via patched `ieee80211_freedom_inside_cb` override
- WPS-IE detection
- Pineapple Mark VII control: dashboard, recon, evil-twin, karma, deauth, handshake browser
- Captive portal (open SoftAP + DNS hijack + form capture)
</details>

<details>
<summary><b>Bluetooth</b></summary>

- BLE scan
- BLE adv-spam (Apple/Samsung/Google nuisance payloads)
- BLE jam (channel saturation, auto-shutoff)
- BLE GATT walk
- Optional dual-rail: spam from S3 NimBLE + Hydra nRF24 simultaneously, channel-divided
- bb-link bridge probe (for the Kenwood TH-D75 path)
</details>

<details>
<summary><b>Sub-GHz (Hydra cap, CC1101)</b></summary>

300–928 MHz with the [Pingequa Hydra RF Cap 424][hydra]. The launcher status strip shows `H+` / `H~` / `H-` so you know if the cap responded at boot.

- Spectrum scan with smart dwell + auto-tune
- Capture / replay (Flipper-compatible `.sub`)
- 24-bit OOK fixed-code brute
- Jammer: CW / Noise / Sweep / Protocol (preamble-IRQ gated) / Reactive
- Edge-toggle async TX with `esp_timer` absolute-deadline timing for rolling-code work
</details>

<details>
<summary><b>2.4 GHz raw (Hydra cap, nRF24L01+)</b></summary>

- Channel-activity scan + heatmap
- Channel jammer
- Mousejack (Logitech Unifying scan + keystroke inject), promiscuous via direct RPD-bit register write
- RollJam state machine
</details>

<details>
<summary><b>Ham radio (Kenwood TH-D75 via bb-link bridge)</b></summary>

- Kenwood: status / pair / refresh
- APRS: RX station list + TX via KISS
- GPS: NMEA from the radio → GPX log on SD
- Remote VFO: freq / mode / step from the keyboard
- SatTracker: TLE + SGP4 + auto-Doppler-correcting CAT during ISS passes
</details>

<details>
<summary><b>Lab Mode</b></summary>

A Faraday-room flag (`Settings → Faraday Mode`) that:

- Strips the politeness gates from the aggressive RF apps
- Removes duty-cycle caps in jammers (Mousejack 50→2 ms, SubGhzBrute 100→10 ms)
- Optionally refuses to enable outside a 50 m radius of a stored lab GPS fix
- Surfaces a red `LAB` badge in the launcher status strip

Plus `RfChaosApp` — random-target RF stress orchestrator, refuses to arm without the flag, logs everything to `/rfchaos/<ts>.log`.

</details>

<details>
<summary><b>System</b></summary>

- SelfTestApp — programmatic 14-step bring-up (one keypress to verify a freshly-flashed board)
- HydraStatusApp — cap presence + radio telemetry
- BbLinkProbeApp — three-phase bridge diagnostic
- SoakLog — appends heap/uptime CSV to SD every minute (paired with `tools/soak-monitor.py`)
- Settings — brightness, theme, timezone, NTP server, Faraday Mode, boot-self-test
- Files (SD browser), Sysinfo, About
</details>

[hydra]: https://www.pingequa.com/products/m5stack-cardputer-adv-2-in-1-rf-module

## First boot

1. Splash → launcher.
2. **WiFi → WiFi**, pick your AP, type the passphrase. NTP sync runs on its own.
3. **System → Settings** for timezone.
4. (TH-D75 owners) **Radio → Kenwood**, Tab to scan for the bb-link bridge, Enter to connect.
5. (Pineapple owners) Set host/user/pass in NVS via serial, then **WiFi → Pineapple**.

After that the device boots straight into the launcher with WiFi already up.

## Hardware

The Cardputer ADV (8 MB / ESP32-S3FN8) on its own runs everything in WiFi, BLE, IR, and the local apps. Add hardware to unlock more:

| Add-on                             | Unlocks                              | Cost                    |
| ---------------------------------- | ------------------------------------ | ----------------------- |
| [Pingequa Hydra RF Cap 424][hydra] | Sub-GHz + 2.4 GHz raw radio category | ~$25                    |
| Kenwood TH-D75 + bb-link bridge    | Ham radio category, SatTracker CAT   | radio price + ~$5 ESP32 |
| Hak5 WiFi Pineapple Mark VII       | Pineapple suite                      | ~$120                   |

## Status

v1.2 shipped. 387 native tests, 32 launcher entries, zero build warnings. Real-hardware bring-up pending — anyone flashing the current head is the first hardware tester. Bring me serial logs if something breaks: [open an issue](https://github.com/leingangzj/yui/issues).

## Disclaimer

Several apps in here transmit RF or inject 802.11 management frames. **Use only on networks, devices, and spectrum you own or are authorized to test.** Deauth, beacon flood, BLE-spam, and captive-portal phishing against unconsenting parties is a federal crime in the US (CFAA, Wiretap Act, FCC Part 15) and the equivalent elsewhere.

This project is for security research and educational use. Bugs happen. If you don't know whether you're allowed to do something, the answer is no.

## Docs

- [`docs/INSTALL.md`](docs/INSTALL.md) — flash the device
- [`docs/USING.md`](docs/USING.md) — what each app does + key bindings
- [`docs/HYDRA.md`](docs/HYDRA.md) — the RF cap
- [`docs/HARDWARE.md`](docs/HARDWARE.md) — Cardputer ADV pinout, peripherals, M5Unified notes
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — HAL-first design, why apps are tiny
- [`docs/ROADMAP.md`](docs/ROADMAP.md) — what shipped, what's next
- [`docs/RELEASING.md`](docs/RELEASING.md) — cut a release
- [`docs/protocols/`](docs/protocols/) — AX.25/APRS, KISS, Pineapple REST, Kenwood SPP, ESP32 WiFi internals

## Prior art

Yui borrows ideas (not code) from [Bruce](https://github.com/BruceDevices/firmware) (AGPL) and [NEMO](https://github.com/n0xa/m5stick-nemo) (GPL). The Bruce-parity sweep was a clean-room rebuild against the same protocol documents — none of their source ever entered this tree.

## License

MIT — see [LICENSE](LICENSE).

## Support

Yui is built and maintained in spare time. If it's useful to you and you'd like to chip in toward more hardware, more features, and the caffeine that makes both happen:

[**☕ Buy me a coffee**](https://buymeacoffee.com/leingangzj)

A star on the repo also helps — makes the project easier to find for the next person.

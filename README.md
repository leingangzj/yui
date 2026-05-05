# Yui

> 結い — _Okinawan: cooperative community work, the bond that ties people together._

Firmware for the M5Stack Cardputer ADV. Aimed at people who already own a
ham radio, a WiFi Pineapple, and an opinion about how a hand-held console
should work.

What it does, in plain terms:

- Talks to a Kenwood TH-D75 over Bluetooth (through a small bb-link bridge,
  because the ESP32-S3 can't do BT-Classic). APRS, GPS, remote VFO control.
- Drives a WiFi Pineapple over its REST API. Recon, evil twin, karma,
  deauth, handshake browser — from the Cardputer's keyboard, no laptop.
- Uses the Cardputer's onboard radio for native 2.4 GHz work: probe-request
  fingerprinting, passive WPA handshake capture, beacon flood, WPS scan,
  captive portal phishing, BLE GATT enumeration, BLE-spam, IR.
- With the **Pingequa Hydra RF Cap 424** plugged into EXT-14, adds dual-
  band sub-GHz + 2.4 GHz (CC1101 + nRF24L01+): spectrum scan, signal
  capture/replay (Flipper-compatible `.sub`), fixed-code brute, sub-GHz
  jammer (CW / noise / sweep), 2.4 GHz channel-flood + hop-jam, Mousejack,
  RollJam state machine. See `docs/HYDRA.md`.
- Tracks satellites. TLE database, next-pass predictor, polar-plot live
  view, and Doppler-correcting CAT control of the TH-D75 so you can work an
  ISS pass without a phone in the loop.

Plus the everyday-carry stuff: notes, files, RPN calculator with trig and
memory, mic FFT, IR remote, calendar, todos, Snake, Life, tone/metronome.

## Status

🛰️ **v1.1 — shipped, awaiting field validation.** `pio run -e
cardputer_adv` and `pio test -e native` are both green. ~50 apps across
6 categories. 362 native tests. v1.1 adds the full Hydra RF stack (HAL

- 9 apps + status diagnostic + cap badge) and prunes 14 utility/game
  apps to focus the device on its RF/security identity; the firmware
  hasn't been validated on real hardware yet — anyone flashing the current
  head is the first hardware tester. If something breaks, please
  [open an issue](https://github.com/leingangzj/yui/issues) with serial
  logs.

## Hardware

You need a Cardputer ADV (8 MB / ESP32-S3FN8). That's it for the basic
build — the launcher, native WiFi/BLE/IR, GPS-via-radio is all unreachable,
but everything else runs.

To unlock the radio category you need a **Kenwood TH-D75** plus a bb-link
bridge ([islandmagic/bb-link](https://github.com/islandmagic/bb-link)).
The bridge is a separate ~$5 ESP32 board running open-source firmware
that translates BLE to BT-Classic SPP, since the Cardputer's S3 is
BLE-only. Pair the bridge to the radio once with their app, then Yui
finds it on its own. See `docs/protocols/KENWOOD_THD75.md` if you want
the protocol detail.

To drive a Pineapple, point a **Hak5 WiFi Pineapple Mark VII** at your
network and put its host + creds in NVS via the serial monitor. Yui
talks to it over the REST API.

For sub-GHz + 2.4 GHz raw RF work (capture/replay/jam/Mousejack/
RollJam), seat a **Pingequa Hydra RF Cap 424** on the EXT-14 expansion
header. The launcher status strip shows `H+` / `H~` / `H-` so you can
confirm the cap responded at boot. Pin map, app catalogue, and
operator notes in `docs/HYDRA.md`.

## Flashing

Three paths, easiest first.

**Web flasher.** Open <https://leingangzj.github.io/yui> in Chrome,
Edge, or any Chromium-based browser on a desktop OS. Plug the device
into a USB-C data port, click Install, pick the serial port, wait
about 90 seconds.

**esptool.** Grab `yui-vX.Y-cardputer_adv.bin` from the
[latest release](https://github.com/leingangzj/yui/releases/latest) and
write it to flash offset `0x0`:

```bash
python3 -m esptool --chip esp32s3 -p /dev/ttyACM0 -b 921600 \
  write_flash --flash_mode qio --flash_freq 80m --flash_size 8MB \
  0x0 yui-v1.1-cardputer_adv.bin
```

**From source.**

```bash
pio test -e native                              # host tests, no hardware
pio run -e cardputer_adv                        # build the firmware
pio run -e cardputer_adv -t upload -t monitor   # flash over USB-C
```

Full instructions including troubleshooting are in `docs/INSTALL.md`.

## First boot

1. Splash → launcher.
2. Open **WiFi → Wifi**, pick your AP, type the passphrase. NTP sync runs
   on its own.
3. **System → Settings** to set timezone.
4. (TH-D75 owners) **Radio → Kenwood**, Tab to scan for the bb-link
   bridge, Enter to connect.
5. (Pineapple owners) Set host/user/pass in NVS via serial, then **WiFi
   → Pineapple** to confirm it's reachable.

After that the device boots straight into the launcher with WiFi already
up, and the rest is whichever app you opened last.

## Architecture

HAL-first C++17. Every byte of hardware access goes through an interface
under `include/yui/hal/`; apps don't import Arduino headers directly.
Native and ESP32 backends both implement those interfaces, which is what
lets the test suite run 423 cases on a desktop machine in five seconds
without any board attached. Hardware checks live on top of that, not in
place of it. See `docs/ARCHITECTURE.md` for the long version, and
`docs/HARDWARE_AUDIT.md` for the pin-by-pin verification against
M5Unified.

## Repo layout

```
art/icons/        Japanese-themed icon set sliced from menu1/menu2 sheets,
                  embedded by tools/gen_assets.py into include/yui/assets/icons.hpp
docs/             ARCHITECTURE, HARDWARE, HARDWARE_AUDIT, INSTALL, USING,
                  ROADMAP, PRIOR_ART, TEST_ENV, HYDRA, plus protocols/
include/yui/
   app/           ~50 app classes (incl. Phase 1-4.9 Hydra RF apps)
   hal/           interface headers — IDisplay, IKeyboard, IRadioLink, IGnss,
                  IHttp, IPcap, IWifiMonitor, IBleAdvertiser, IBleCentral,
                  IWifiAp, IFs, IClock, ILog, INet, IImu, IIr, IMic, ISpeaker,
                  IStorage, ICc1101, INrf24
   proto/         AX.25, APRS, KISS, Dot11, Pcap, Pineapple, SubFile decoders
   sat/           Tle, Vec3, Time, Propagator, Topo, Pass — SatTracker math
src/hal/native/   Fake* backends used by the host tests
src/hal/esp32/    Esp32* backends against M5Unified, NimBLE, esp_wifi, etc.,
                  plus Cc1101Radio + Nrf24Radio (RadioLib over HydraSpiBus)
test/test_native/ all 423 native tests, currently in one big file
tools/            extract_icons.py, gen_assets.py, pack-release.sh,
                  bringup-checklist.md, soak-monitor.py
platformio.ini    `native` and `cardputer_adv` envs
```

## License

MIT — see [LICENSE](LICENSE). Keep the copyright notice, otherwise have at it.

## Prior art

Yui borrows ideas (not code) from [Bruce](https://github.com/BruceDevices/firmware)
(AGPL) and [NEMO](https://github.com/n0xa/m5stick-nemo) (GPL). The
Bruce-parity sweep was a clean-room rebuild against the same protocol
documents Bruce works from — none of their source ever entered this tree.

## Legal

Several apps in here transmit RF or inject 802.11 management frames.
**Only point them at spectrum, networks, and devices you own or have
written authorization to test.** Deauth, beacon flood, BLE-spam, and
captive-portal phishing against unconsenting parties is a federal
crime in the US (CFAA, Wiretap Act, FCC Part 15) and the equivalent
elsewhere. If you don't know whether you're allowed to do something,
the answer is no.

## Roadmap

`docs/ROADMAP.md`. Short version: v0.1 / v0.2 / v0.3 / v1.0 / v1.1 all
shipped in the firmware. v1.2 (Lab Mode) and v1.3 (LAN/Service Recon)
are spec'd. Real-hardware validation is the next thing.

## Support development

Yui is built and maintained in spare time. If it's useful to you and
you'd like to chip in toward more hardware, more features, and the
caffeine that makes both happen:

[**☕ Buy me a coffee**](https://buymeacoffee.com/leingangzj)

Stars on the repo also help — they make the project easier to find for
the next person.

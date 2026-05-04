# Hydra RF Cap 424 — operator's guide

[Pingequa Hydra RF Cap 424][pq] is a dual-band RF expansion for the
Cardputer ADV: a TI **CC1101** sub-GHz radio (300/433/868/915 MHz) and
a Nordic **nRF24L01+** 2.4 GHz radio share one SPI bus, with auto-
switching of CS handled on the cap. This page covers how Yui drives it.

[pq]: https://www.pingequa.com/products/m5stack-cardputer-adv-2-in-1-rf-module

## Pin map

```
Shared SPI bus (HSPI):      SCK = GPIO 40
                            MOSI = GPIO 14   (also SD card)
                            MISO = GPIO 39

CC1101 (sub-GHz):           CS  = GPIO 13
                            io0 = GPIO 5     (GDO0 — data + IRQ)
                            io2 = unused (-1)

nRF24L01+ (2.4 GHz):        CS  = GPIO 6
                            io0 = GPIO 4     (chip-enable / CE)
                            io2 = unused (-1)
```

Constants: `include/yui/config/pins.hpp` under "Pingequa Hydra RF Cap
424". Pingequa's brucePins.conf uses the same symbols (cs / io0 /
io2) — the `io0` slot carries CC1101's GDO0 on the sub-GHz side and
nRF24's CE on the 2.4 GHz side.

## HAL

Two abstract interfaces in `include/yui/hal/`:

| Interface | What it covers                                        |
| --------- | ----------------------------------------------------- |
| `ICc1101` | freq + modulation + bitrate, RSSI, RX/TX, carrier     |
| `INrf24`  | channel + data rate + address, listen, RX/TX, carrier |

Concrete impls in `src/hal/esp32/`:

- `Cc1101Radio.hpp` — RadioLib-backed CC1101 on the shared bus
- `Nrf24Radio.hpp` — RadioLib-backed nRF24 on the shared bus
- `HydraSpiBus.hpp` — singleton `SPIClass(HSPI)` both chips share

Native fakes in `src/hal/native/`:

- `NativeCc1101.hpp` — present-flag toggle, freq band check, RX
  queue, last-TX inspect, carrier toggle, RSSI canned values
- `NativeNrf24.hpp` — same shape for nRF24 (channel/address-width
  validation, listen state, carrier toggle)

## Boot probe

`main.cpp` probes both chips after `store_.init()`:

```cpp
cc1101_present_ = cc1101_.begin();
nrf24_present_  = nrf24_.begin();
log_.info("[hydra] cc1101=%s nrf24=%s ...", ...);
```

`begin()` issues the chip's standard SPI handshake. If the chip
doesn't ACK, the present flag stays false, every RF app picks up the
nullable pointer and renders `Chrome::dialog` "Hydra not found" on
entry instead of crashing.

## Apps

### Sub-GHz (CC1101) — Category::Radio

| App           | What it does                                                                                                                                         |
| ------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| SubGhzScan    | RSSI sweep across 4 ISM bands (315 / 433 / 868 / 915). 60 bins, peak highlight, Left/Right band-flip, Enter pause/resume.                            |
| SubGhzCapture | Records to `/sub/<freq>_<ts>.sub` (Flipper format). 30 s safety stop, 4096-edge cap.                                                                 |
| SubGhzReplay  | File picker for `/sub/*.sub`. Loads, retunes to the file's frequency, packs RAW timings, transmits via packet mode.                                  |
| SubGhzBrute   | 24-bit fixed-code dictionary walker. Configurable freq + step, paced 10 Hz TX, 3-byte big-endian payload.                                            |
| SubGhzJammer  | CW (carrier) / Noise (LCG-random TX) / Sweep (saw-tooth ±500 kHz) / Protocol / Reactive. First three live; last two stubbed for preamble-IRQ wiring. |
| RollJam       | Idle → Jamming → Captured → Replayed → Idle. Real carrier toggle + capture/replay; tight rolling-code timing TBD.                                    |

### 2.4 GHz (nRF24) — Category::Bluetooth

| App         | What it does                                                                                                                                                         |
| ----------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Nrf24Scan   | 126-channel activity heatmap with decay. 10 ch/tick.                                                                                                                 |
| Nrf24Jammer | Channel flood (single-channel carrier) / Hop jam (cycles 0..125 per tick) / Target MAC (stubbed).                                                                    |
| Mousejack   | Channel-walk scan (seeded with Logitech Unifying address) → target list → paced HID-payload inject. Promiscuous-mode requires low-level register poke not yet wired. |

## File formats

### `.sub` — Flipper-compatible RAW

`include/yui/proto/SubFile.hpp` writes and reads the Flipper Zero
sub-GHz capture format:

```
Filetype: Flipper SubGhz RAW File
Version: 1
Frequency: 433920000
Preset: FuriHalSubGhzPresetOok650Async
Protocol: RAW
RAW_Data: 320 -130 196 -118 152 -86 ...
```

Captures saved by `SubGhzCaptureApp` drop into the Flipper community
signal library as-is, and signals downloaded from there feed
`SubGhzReplayApp` directly.

`SubPreset` covers `Ook650Async`, `Ook270Async`, `FskDev238Async`,
`FskDev476Async`, `Custom`. Reader is allocation-friendly (callback
per timing); writer takes a span + `SubHeader`.

## Pentesting boundaries

The Cardputer ADV with the Hydra is a Flipper-class device. Yui does
not implement region locks or TX power limits — the device is
expected to live in a Faraday cage for testing, per the operator's
own infrastructure.

If you flash it onto a Cardputer that's not in a contained
environment, **don't operate the jammer / brute / inject apps over
the air.** The HAL will happily transmit; the radios don't ask
permission. That is the operator's responsibility.

## Phase log (where this came from)

- **Phase 3** — `ICc1101` / `INrf24` interfaces, RadioLib-backed
  ESP32 impls, native fakes, `HydraSpiBus`, `.sub` format, boot
  probe in `main.cpp`. (commit `276aecc`)
- **Phase 4** — 9 RF apps wired into the launcher; 3 fully working,
  6 cap-detection scaffolds. (commit `394b323`)
- **Phase 4.5** — Replay + Brute fully wired, Jammer Noise + Sweep
  modes. (commit `429eb02`)
- **Phase 4.6** — Mousejack scan/inject + RollJam state machine
  drive real radio ops. (commit `70b25d9`)

## Future work (waiting on hardware-test signal)

- **Promiscuous-mode nRF24 sniff** — needed for Mousejack against
  arbitrary Logitech dongles. Requires direct register write
  outside RadioLib's public API.
- **Edge-toggle async sub-GHz TX** — for Flipper-perfect rolling-
  code replay. Needs GDO0 toggling in tight loops.
- **Preamble-detect IRQs** — unlocks Jammer Protocol / Reactive
  modes and tighter RollJam timing.
- **`SubGhzScanApp` smarter dwell** — currently 1 sample per tick;
  hardware test will tell us whether longer dwell catches more
  bursts.

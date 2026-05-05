# Hydra RF Cap 424 — operator's guide

The [Pingequa Hydra RF Cap 424][pq] is a dual-band RF expansion for the Cardputer ADV. A TI **CC1101** sub-GHz radio (300 / 433 / 868 / 915 MHz) and a Nordic **nRF24L01+** 2.4 GHz radio share one SPI bus. Auto-switching of CS happens on the cap. This page is how Yui drives it.

[pq]: https://www.pingequa.com/products/m5stack-cardputer-adv-2-in-1-rf-module

## Pin map

```
Shared SPI bus (HSPI):    SCK  = GPIO 40
                          MOSI = GPIO 14   (also SD card)
                          MISO = GPIO 39

CC1101 (sub-GHz):         CS   = GPIO 13
                          io0  = GPIO 5    (GDO0 — data + IRQ)
                          io2  = unused

nRF24L01+ (2.4 GHz):      CS   = GPIO 6
                          io0  = GPIO 4    (CE)
                          io2  = unused
```

Constants in `include/yui/config/pins.hpp`. Pingequa's `brucePins.conf` uses the same naming (cs / io0 / io2) — `io0` carries CC1101's GDO0 on the sub-GHz side and nRF24's CE on the 2.4 GHz side.

## HAL

| Interface | Covers                                                                                       |
| --------- | -------------------------------------------------------------------------------------------- |
| `ICc1101` | freq + modulation + bitrate, RSSI, RX/TX, carrier, edge-toggle async TX, packet/preamble IRQ |
| `INrf24`  | channel + data rate + address, listen, RX/TX, carrier, RPD-bit promiscuous mode              |

Concrete impls in `src/hal/esp32/`:

- `Cc1101Radio.hpp` — RadioLib-backed CC1101 on the shared bus
- `Nrf24Radio.hpp` — RadioLib-backed nRF24 on the shared bus
- `HydraSpiBus.hpp` — singleton `SPIClass(HSPI)` both chips share

Native fakes in `src/hal/native/`: `NativeCc1101.hpp`, `NativeNrf24.hpp`. Both expose test seams (`set_present`, `enqueue_rx`, `simulate_packet_irq`, `force_promiscuous_fail`, etc.) so the host test suite can exercise everything without real hardware.

## Boot probe

`main.cpp` probes both chips after `store_.init()`:

```cpp
cc1101_present_ = cc1101_.begin();
nrf24_present_  = nrf24_.begin();
```

`begin()` issues the chip's standard SPI handshake. If the chip doesn't ACK, the present flag stays false. Every RF app picks up the nullable pointer and renders `Chrome::cap_missing_dialog` ("Hydra not found") instead of crashing.

The launcher status strip shows `H+` / `H~` / `H-` (both up / one up / cap absent) so you know without opening anything.

## Apps

### Sub-GHz (CC1101)

| App                     | What it does                                                                                                                                                                                                  |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Sub-GHz Scan**        | RSSI sweep across 4 ISM bands (315 / 433 / 868 / 915). 60 bins, peak highlight, ^/v changes band, Enter pause/resume. Smart dwell (5 samples/bin); auto-tune doubles dwell on quiet bins, halves on hot ones. |
| **Sub-GHz Capt/Replay** | Tab cycles Read ↔ Saved. Read records to `/sub/<freq>_<ts>.sub` (Flipper format), 30s safety stop, 4096-edge cap. Saved is a file picker → load → transmit via edge-toggle async TX.                          |
| **Sub-GHz Brute**       | 24-bit fixed-code dictionary walker. Configurable freq + step. Pace 100 ms (10 ms in Faraday Mode).                                                                                                           |
| **Sub-GHz Jam**         | CW / Noise / Sweep / Protocol / Reactive. Protocol/Reactive register the packet IRQ and only fire CW when a target preamble is heard — much stealthier than continuous CW.                                    |
| **RollJam**             | Idle → Jamming → Captured → Replayed → Idle. Real carrier toggle + capture/replay; tight rolling-code timing pending hardware validation.                                                                     |

### 2.4 GHz (nRF24)

| App           | What it does                                                                                                                                                                                                                                                                                                                                        |
| ------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Mousejack** | Channel-walk scan (seeded with Logitech Unifying address) → target list → paced HID-payload inject. Tab toggles to **Generic Scan** mode (the absorbed Nrf24Scan path: 126-channel heatmap with decay). Promiscuous mode pokes register 0x09 directly + reads back; surfaces failure if the chip refuses. Inject pace 50 ms (2 ms in Faraday Mode). |
| **NRF24 Jam** | Channel flood, hop jam, target MAC. ^/v changes channel.                                                                                                                                                                                                                                                                                            |

### Cross-band

| App              | What it does                                                                                                                                                                                                   |
| ---------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Hydra Status** | Cap presence + per-radio telemetry (freq, channel, RX/TX bytes). Faraday Mode state shown here too.                                                                                                            |
| **RF Chaos**     | Lab-only orchestrator. Random-target firing across BLE / nRF24 / CC1101 / WiFi-monitor for 1–60 min at intensity 1–10. Refuses to arm without Faraday Mode. Logs (ms, target, channel) to `/rfchaos/<ts>.log`. |

## Edge-toggle async TX

`Cc1101Radio::transmit_raw_edges()` bit-bangs GDO0 directly with absolute-deadline timing using `esp_timer_get_time()` so cumulative jitter doesn't drift. For edges >50 µs we coarse-delay first, then tight-spin the last 20 µs to the deadline. Worst-edge slip is tracked and exposed via `last_raw_tx_jitter_us()` — under 10 µs is Flipper-perfect for typical 433 MHz fixed-code remotes; above 25 µs starts to risk rolling-code timing windows.

## Packet/preamble IRQ

`ICc1101::on_packet_irq(callback, ctx)` registers a GDO0 GPIO ISR. Used by SubGhzJammer's Protocol/Reactive modes today; available for tighter RollJam timing in the future. Native fake exposes `simulate_packet_irq()` so the path is testable without a wire.

## .sub file format (Flipper-compatible)

`include/yui/proto/SubFile.hpp` reads and writes:

```
Filetype: Flipper SubGhz RAW File
Version: 1
Frequency: 433920000
Preset: FuriHalSubGhzPresetOok650Async
Protocol: RAW
RAW_Data: 320 -130 196 -118 152 -86 ...
```

Captures saved by Yui drop into the Flipper community signal library as-is, and signals downloaded from there feed Replay directly.

`SubPreset` covers `Ook650Async`, `Ook270Async`, `FskDev238Async`, `FskDev476Async`, `Custom`.

## Pentesting boundaries

The Cardputer ADV with the Hydra is a Flipper-class device. Yui does not implement region locks or TX power limits — the device is expected to live in a Faraday cage for testing.

If you flash it onto hardware that isn't in a contained environment, **don't operate the jammer / brute / inject apps over the air.** The HAL will transmit; the radios don't ask permission. That's the operator's call. Pair with **Faraday Mode** (Settings) so the strip is conditional on a controlled RF environment.

## Phase log

- **Phase 3** (`276aecc`) — `ICc1101` / `INrf24` interfaces, RadioLib-backed ESP32 impls, native fakes, `HydraSpiBus`, `.sub` format, boot probe.
- **Phase 4** (`394b323`) — 9 RF apps in the launcher; 3 fully working, 6 cap-detection scaffolds.
- **Phase 4.5** (`429eb02`) — Replay + Brute fully wired, Jammer Noise + Sweep modes.
- **Phase 4.6** (`70b25d9`) — Mousejack scan/inject + RollJam state machine drive real radio ops.
- **Phase 5 (Lab Mode)** — promiscuous-mode RPD read-back, edge-toggle TX with `esp_timer` + jitter trace, packet-IRQ HAL → Protocol/Reactive jammer modes, smart dwell + auto-tune for SubGhzScan.

## Still pending hardware validation

- Mousejack against arbitrary Logitech dongles in promiscuous mode (the register poke is shipped, the latch behavior needs a wire to confirm).
- Flipper-perfect rolling-code replay timing across more remote vendors (the edge-toggle path is shipped + jitter-traced; field signal needed).
- Protocol/Reactive jammer trigger latency under real preamble-detect IRQ.
- SubGhzScan auto-tune dwell calibration against a known-noisy band.

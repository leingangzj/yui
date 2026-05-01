# Hardware: M5Cardputer ADV

Verified against the [official M5Stack docs](https://docs.m5stack.com/en/core/Cardputer%20ADV).

## SoC & Memory

- **MCU:** ESP32-S3FN8, dual-core Xtensa LX7, up to 240 MHz
- **Flash:** 8 MB
- **PSRAM:** None on this SKU (the FN8 part has no embedded PSRAM)
- **Storage:** microSD via SPI

## Display & Input

- **LCD:** 1.14" ST7789V2, 240×135 px, SPI
- **Keyboard:** 56 keys (4×14), driven by TCA8418RTWR I²C keypad controller
  _(this differs from the base Cardputer, which scans GPIO directly)_
- **Actuation:** 160 gf

## Wireless

- **WiFi 4** (ESP32-S3 integrated)
- **Bluetooth 5.0 / BLE** (ESP32-S3 integrated)
- **No onboard LoRa, NFC, sub-GHz, or cellular.** LoRa+GNSS comes from the
  optional Cap LoRa-1262 ($14.50) or Mesh Kit ($48).

## Sensors

- **BMI270** 6-axis IMU (accel + gyro)
- **MEMS microphone** (65 dB SNR)
- _No environmental, light, or CO₂ sensors onboard._

## Audio

- **ES8311** codec + **NS4150B** amplifier + 8 Ω / 1 W speaker
- **3.5 mm audio jack**

## Other I/O

- **IR emitter** (1×)
- **Grove HY2.0-4P** port
- **EXT 2.54 mm 14-pin** expansion bus (SPI / I²C / UART / GPIO)

## Power

- 1750 mAh Li-ion, USB-C charging

## PlatformIO Board

```ini
board = esp32-s3-devkitc-1
```

The official M5 docs use the generic ESP-IDF board. There is no dedicated
`m5stack-cardputer-adv` board ID in mainline `platform-espressif32` as of
2026-05; we override pins via build flags rather than relying on a board pin
map.

## Pin Map (working draft — confirm against silkscreen on first power-up)

> ⚠️ **Verify these against the Cardputer ADV schematic before relying on
> them.** The base-Cardputer pin map circulating online does not match the ADV
> in several places (TCA8418 keyboard, ES8311 audio routing, EXT-14 vs EXT-9).

### Internal I²C (kbd controller, IMU, audio codec)

| Signal    | GPIO | Notes                             |
| --------- | ---- | --------------------------------- |
| `INT_SDA` | TBD  | Shared by TCA8418, BMI270, ES8311 |
| `INT_SCL` | TBD  |                                   |

### Display SPI

| Signal     | GPIO | Notes         |
| ---------- | ---- | ------------- |
| `LCD_MOSI` | TBD  |               |
| `LCD_SCK`  | TBD  |               |
| `LCD_DC`   | TBD  |               |
| `LCD_RST`  | TBD  |               |
| `LCD_CS`   | TBD  |               |
| `LCD_BL`   | TBD  | Backlight PWM |

### microSD SPI

| Signal                                     | GPIO | Notes                          |
| ------------------------------------------ | ---- | ------------------------------ |
| `SD_MOSI` / `SD_MISO` / `SD_SCK` / `SD_CS` | TBD  | Likely shares bus with display |

### IR

| Signal  | GPIO | Notes                       |
| ------- | ---- | --------------------------- |
| `IR_TX` | TBD  | Single emitter, no receiver |

### Mic / Audio

| Signal                                | GPIO | Notes     |
| ------------------------------------- | ---- | --------- |
| `MIC_DATA` (PDM or I²S)               | TBD  |           |
| `I2S_BCLK` / `I2S_LRCLK` / `I2S_DOUT` | TBD  | To ES8311 |

### Grove HY2.0-4P (external I²C / UART)

| Pin | Signal                  | GPIO |
| --- | ----------------------- | ---- |
| 1   | VCC (5V)                | —    |
| 2   | GND                     | —    |
| 3   | `GROVE_SDA` / `UART_TX` | TBD  |
| 4   | `GROVE_SCL` / `UART_RX` | TBD  |

### EXT 14-pin expansion

Pinout TBD — confirm from official ADV schematic. This bus is what Caps
(LoRa-1262, etc.) plug into.

---

## Open Hardware Tasks

- [ ] Pull the ADV schematic PDF from M5Stack and fill the GPIO column above.
- [ ] Verify TCA8418 I²C address (typically `0x34`).
- [ ] Verify BMI270 I²C address (`0x68` or `0x69` depending on SDO strap).
- [ ] Verify ES8311 I²C address (typically `0x18`).
- [ ] Confirm whether Grove and EXT-14 share buses with anything internal.

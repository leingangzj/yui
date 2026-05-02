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

## Pin Map

Sourced from M5Stack's official Cardputer-ADV docs. Values are baked into
`include/yui/config/pins.hpp` for use from C++.

### Internal I²C (TCA8418 kbd · BMI270 IMU · ES8311 codec)

| Signal    | GPIO   |
| --------- | ------ |
| `INT_SDA` | GPIO 8 |
| `INT_SCL` | GPIO 9 |

I²C addresses: TCA8418 = `0x34`, BMI270 = `0x68`, ES8311 = `0x18`.

### Display SPI (ST7789V2, 240×135)

| Signal     | GPIO    |
| ---------- | ------- |
| `LCD_MOSI` | GPIO 35 |
| `LCD_SCK`  | GPIO 36 |
| `LCD_CS`   | GPIO 37 |
| `LCD_DC`   | GPIO 34 |
| `LCD_RST`  | GPIO 33 |
| `LCD_BL`   | GPIO 38 |

### microSD SPI

| Signal    | GPIO    |
| --------- | ------- |
| `SD_MOSI` | GPIO 14 |
| `SD_MISO` | GPIO 39 |
| `SD_CLK`  | GPIO 40 |
| `SD_CS`   | GPIO 12 |

### IR

| Signal  | GPIO    |
| ------- | ------- |
| `IR_TX` | GPIO 44 |

### Audio (ES8311 codec)

| Signal             | GPIO    |
| ------------------ | ------- |
| `I2S_SCLK`         | GPIO 41 |
| `I2S_LRCK`         | GPIO 43 |
| `I2S_DSDIN` (out)  | GPIO 42 |
| `I2S_ASDOUT` (mic) | GPIO 46 |

### Battery ADC

| Signal    | GPIO    |
| --------- | ------- |
| `BAT_ADC` | GPIO 10 |

### Grove HY2.0-4P (external)

| Pin | Signal      | GPIO   |
| --- | ----------- | ------ |
| 1   | VCC (5V)    | —      |
| 2   | GND         | —      |
| 3   | `GROVE_SDA` | GPIO 2 |
| 4   | `GROVE_SCL` | GPIO 1 |

### EXT 14-pin (where Caps like LoRa-1262 plug in)

Physical pin layout (two columns, looking at the connector from the top):

| Pins  | Left (odd)     | Right (even)      |
| ----- | -------------- | ----------------- |
| 1-2   | RESET — GPIO 3 | 5VIN              |
| 3-4   | INT — GPIO 4   | GND               |
| 5-6   | BUSY — GPIO 6  | 5VOUT             |
| 7-8   | SCK — GPIO 40  | I²C_SDA — GPIO 8  |
| 9-10  | MOSI — GPIO 14 | I²C_SCL — GPIO 9  |
| 11-12 | MISO — GPIO 39 | UART_RX — GPIO 13 |
| 13-14 | CS — GPIO 5    | UART_TX — GPIO 15 |

> ⚠️ **Bus sharing:** the EXT-14 I²C pins are physically the same bus as the
> internal I²C (GPIO 8/9). Caps must use I²C addresses that don't collide
> with `0x18`, `0x34`, `0x68`. SD card and EXT-14 share the SPI bus
> (CLK/MOSI/MISO on 40/14/39); only CS pins differ.

## Keyboard Matrix (TCA8418, 4×14 logical layout)

Per M5's reference, TCA8418 is configured as a 7×8 matrix; their `remap()`
maps raw `(raw_row, raw_col)` to logical `(row, col)`:

```
logical_col = raw_row * 2 + (raw_col > 3 ? 1 : 0)
logical_row = (raw_col + 4) % 4
```

The logical 4×14 layout (row 0 = top):

```
Row 0: ` 1 2 3 4 5 6 7 8 9 0 - = del
Row 1: tab q w e r t y u i o p [ ] \
Row 2: fn  shift a s d f g h j k l ;(↑) ' enter
Row 3: ctrl opt alt z x c v b n m ,(←) .(↓) /(→) space
```

Fn-modified keys (when Fn held):

- Row 0 `` ` `` → `esc`, `del` → `Delete (forward)`
- Row 2 `;` → `up arrow`
- Row 3 `,` → `left`, `.` → `down`, `/` → `right`

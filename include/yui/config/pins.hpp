#pragma once
// M5Cardputer-ADV pin assignments (verified against official M5Stack docs).
// See docs/HARDWARE.md for the source table and notes about bus sharing.

namespace yui::pins {

// Internal I²C (TCA8418 keypad, BMI270 IMU, ES8311 audio codec)
constexpr int kIntSda = 8;
constexpr int kIntScl = 9;

// TCA8418 keypad INT line — asserts low when key-event FIFO is non-empty.
// Used (when wired) to skip I²C reads on a quiet keyboard. Optional.
constexpr int kKbdInt = 11;

// Display ST7789V2
constexpr int kLcdMosi = 35;
constexpr int kLcdSck  = 36;
constexpr int kLcdCs   = 37;
constexpr int kLcdDc   = 34;
constexpr int kLcdRst  = 33;
constexpr int kLcdBl   = 38;

// microSD (shares SPI with EXT-14)
constexpr int kSdMosi = 14;
constexpr int kSdMiso = 39;
constexpr int kSdClk  = 40;
constexpr int kSdCs   = 12;

// IR emitter
constexpr int kIrTx = 44;

// I²S to ES8311
constexpr int kI2sSclk    = 41;
constexpr int kI2sLrck    = 43;
constexpr int kI2sDsdin   = 42;  // codec → speaker
constexpr int kI2sAsdout  = 46;  // mic → codec → MCU

// Battery voltage ADC
constexpr int kBatAdc = 10;

// Grove HY2.0-4P (external)
constexpr int kGroveSda = 2;
constexpr int kGroveScl = 1;

// EXT-14 selected pins
constexpr int kExtReset = 3;
constexpr int kExtInt   = 4;
constexpr int kExtBusy  = 6;
constexpr int kExtCs    = 5;
constexpr int kExtUartRx = 13;
constexpr int kExtUartTx = 15;

// I²C addresses
constexpr unsigned char kI2cAddrTca8418 = 0x34;
constexpr unsigned char kI2cAddrBmi270  = 0x68;
constexpr unsigned char kI2cAddrEs8311  = 0x18;

// ── Pingequa Hydra RF Cap 424 (CC1101 + nRF24 dual-band) ────────────
// Sits on the EXT-14 expansion connector. Both chips share SCK/MOSI/MISO
// with the microSD bus; auto-switching of CS happens on the cap itself,
// so we just drive the per-chip CS + io0 pins from the host.
//
// Pingequa's official brucePins.conf:
//   CC1101: cs=13, io0=5  (io0 carries GDO0 for CC1101)
//   NRF24:  cs=6,  io0=4  (io0 carries the chip-enable pin)
constexpr int kHydraSck   = 40;   // shared with kSdClk
constexpr int kHydraMosi  = 14;   // shared with kSdMosi
constexpr int kHydraMiso  = 39;   // shared with kSdMiso
constexpr int kCc1101Cs   = 13;
constexpr int kCc1101Io0  = 5;    // GDO0 — data + IRQ
constexpr int kCc1101Io2  = -1;   // unused on this cap
constexpr int kNrf24Cs    = 6;
constexpr int kNrf24Io0   = 4;    // chip-enable (CE) on NRF24 side
constexpr int kNrf24Io2   = -1;   // unused

}  // namespace yui::pins

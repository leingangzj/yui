#pragma once
// M5Cardputer-ADV pin assignments (verified against official M5Stack docs).
// See docs/HARDWARE.md for the source table and notes about bus sharing.

namespace yui::pins {

// Internal I²C (TCA8418 keypad, BMI270 IMU, ES8311 audio codec)
constexpr int kIntSda = 8;
constexpr int kIntScl = 9;

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

}  // namespace yui::pins

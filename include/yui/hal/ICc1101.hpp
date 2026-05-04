#pragma once
// ICc1101 — sub-GHz radio HAL (Texas Instruments CC1101).
//
// On Cardputer ADV with the Pingequa Hydra RF Cap 424, the chip lives
// on the EXT-14 expansion bus. Pin map: see yui::pins::kCc1101Cs/Io0.
//
// All radios are nullable: the cap may not be plugged in. Apps that
// need sub-GHz check `hal.cc1101 != nullptr` (or whatever pointer
// they're handed) and show a friendly "cap not detected" dialog if
// it's missing. The Shell does this gating automatically once we
// route apps through the registry.

#include <cstdint>
#include <cstddef>

namespace yui {

// Modulation types we care about. Not exhaustive — these are the four
// that cover ~all sub-GHz pentest work (garage doors, weather, TPMS,
// IoT remotes).
enum class CcModulation : uint8_t {
  Ook,    // on-off keying — the most common 433 MHz remote modulation
  Ask,    // amplitude-shift keying (close cousin of OOK)
  Fsk2,   // 2-FSK (weather sensors, some IoT)
  Fsk4,   // 4-FSK
};

class ICc1101 {
public:
  virtual ~ICc1101() = default;

  // ── Lifecycle ────────────────────────────────────────────────────
  // begin() probes the chip via SPI. Returns true if the part-number
  // register reads as expected. After begin() == true, is_present()
  // stays true until the cap is unplugged (we re-probe on demand).
  virtual bool begin() = 0;
  virtual bool is_present() = 0;
  virtual void end() {}  // power-down / release SPI; default = no-op

  // Last error string, useful for the cap-detect dialog. Pointer must
  // outlive the call (impls return string literals).
  virtual const char* last_error() const { return ""; }

  // ── Tuning ──────────────────────────────────────────────────────
  // Frequency in Hz (e.g. 433920000 for 433.92 MHz). Returns false if
  // the freq is outside the chip's supported bands.
  virtual bool set_frequency_hz(uint32_t hz) = 0;
  virtual uint32_t frequency_hz() const = 0;

  // Modulation + data rate (bps). Defaults are vendor-typical.
  virtual bool set_modulation(CcModulation m) = 0;
  virtual bool set_bitrate_bps(uint32_t bps) = 0;

  // ── Receive ─────────────────────────────────────────────────────
  // Read current RSSI in dBm (typ. -120 .. -20). Returns INT16_MIN if
  // not in receive mode.
  virtual int16_t read_rssi_dbm() = 0;

  // Pull a single received packet (buffer must be ≥ max). Returns the
  // number of bytes copied, 0 if no packet pending, -1 on chip error.
  virtual int receive(uint8_t* buf, std::size_t max) = 0;

  // ── Transmit ────────────────────────────────────────────────────
  // Push a packet over the air. Synchronous — returns when the chip's
  // FIFO has flushed. true on success.
  virtual bool transmit(const uint8_t* data, std::size_t len) = 0;

  // Continuous-wave (carrier-only) transmit at the current frequency.
  // Used by SubGhzJammerApp's CW mode. on=true asserts carrier; on=false
  // returns the chip to standby.
  virtual bool set_carrier(bool on) = 0;

  // ── Raw edge-toggle TX ──────────────────────────────────────────
  // Transmit a sequence of µs edge timings (positive = TX-on,
  // negative = TX-off — the Flipper RAW format). Goes through the
  // chip's async TX direct mode + tight GPIO bit-bang on GDO0, so
  // timing-perfect rolling-code replay works on receivers that won't
  // accept the packet-mode framing transmit() uses.
  //
  // Default impl falls back to packet-mode transmit() for backends
  // that don't have an async path — gives apps a sane baseline.
  virtual bool transmit_raw_edges(const int32_t* timings_us,
                                  std::size_t n) {
    // Fallback: pack the edges as bits (sign → on/off) and TX as a
    // standard packet. Same compromise SubGhzReplay used in 4.5.
    if (!timings_us || n == 0) return true;
    uint8_t buf[256];
    std::size_t bytes = 0;
    uint8_t cur = 0;
    int bit = 7;
    for (std::size_t i = 0; i < n && bytes < sizeof(buf); ++i) {
      if (timings_us[i] > 0) cur |= (1 << bit);
      if (--bit < 0) { buf[bytes++] = cur; cur = 0; bit = 7; }
    }
    if (bit != 7 && bytes < sizeof(buf)) buf[bytes++] = cur;
    return transmit(buf, bytes);
  }

  // ── Telemetry ───────────────────────────────────────────────────
  // Bytes received / transmitted across this radio's lifetime. Tests
  // assert against these.
  virtual uint64_t rx_bytes() const = 0;
  virtual uint64_t tx_bytes() const = 0;
};

}  // namespace yui

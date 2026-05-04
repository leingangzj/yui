#pragma once
// INrf24 — 2.4 GHz radio HAL (Nordic Semiconductor nRF24L01+).
//
// On Cardputer ADV with the Pingequa Hydra RF Cap 424, this chip
// shares SPI with the CC1101 sub-GHz radio. Pin map: see
// yui::pins::kNrf24Cs/Io0 (Pingequa labels CE as `io0` in their
// brucePins.conf — same pin slot Bruce uses for CC1101's GDO0).
//
// Use cases:
//   • 2.4 GHz channel scanner (SubGhzScanApp / Nrf24ScanApp)
//   • Logitech Mousejack-style HID injection (MousejackApp)
//   • Channel-flood and hop-jam (Nrf24JammerApp)
//   • Drone / RC remote sniffing (custom packet RX)

#include <cstdint>
#include <cstddef>

namespace yui {

// Air data rate. nRF24 supports three rates; pentest tooling typically
// uses 1 Mbps for compatibility (Logitech Unifying, generic dongles)
// and 2 Mbps for higher-throughput sniffing.
enum class NrfDataRate : uint8_t {
  Rate250kbps,
  Rate1Mbps,
  Rate2Mbps,
};

class INrf24 {
public:
  virtual ~INrf24() = default;

  // ── Lifecycle ────────────────────────────────────────────────────
  virtual bool begin() = 0;
  virtual bool is_present() = 0;
  virtual void end() {}

  virtual const char* last_error() const { return ""; }

  // ── Channel / data rate ─────────────────────────────────────────
  // Channel 0..125 → frequency = 2400 + ch MHz. Returns false if the
  // requested channel is out of range.
  virtual bool set_channel(uint8_t channel) = 0;
  virtual uint8_t channel() const = 0;

  virtual bool set_data_rate(NrfDataRate r) = 0;

  // Address width / pipe-0 address. nRF24 packets are addressed by an
  // up-to-5-byte pipe ID. For sniffing we set a known prefix; for
  // injection we set the target dongle's address.
  virtual bool set_address(const uint8_t* addr, std::size_t len) = 0;

  // ── Receive ─────────────────────────────────────────────────────
  virtual bool start_listening() = 0;
  virtual bool stop_listening() = 0;

  // Quick activity check — true if the carrier-detect bit is set on
  // the current channel. Used by channel-scanner apps.
  virtual bool carrier_detected() = 0;

  // Pull a packet. Returns bytes copied, 0 if none pending, -1 on err.
  virtual int receive(uint8_t* buf, std::size_t max) = 0;

  // ── Transmit ────────────────────────────────────────────────────
  virtual bool transmit(const uint8_t* data, std::size_t len) = 0;

  // Continuous carrier on the current channel. Used by Nrf24JammerApp
  // for narrowband flood. on=true asserts carrier.
  virtual bool set_carrier(bool on) = 0;

  // ── Promiscuous mode (Mousejack) ────────────────────────────────
  // Reconfigure the chip so it receives any frame on the current
  // channel regardless of address — the Bastille trick that makes
  // a real Logitech-Unifying scan work. Implementation pokes the
  // chip's registers directly (RX_ADDR_P0=0xAA, AW=2, EN_AA=0,
  // RX_PW_P0=32) outside RadioLib's normal API.
  //
  // Default impl returns false — backends that don't support
  // promiscuous mode signal that scanning is limited to seeded
  // addresses. Apps check the return and degrade gracefully.
  virtual bool set_promiscuous(bool /*on*/) { return false; }
  virtual bool is_promiscuous() const { return false; }

  // ── Telemetry ───────────────────────────────────────────────────
  virtual uint64_t rx_bytes() const = 0;
  virtual uint64_t tx_bytes() const = 0;
};

}  // namespace yui

#pragma once
// IBleRawTx — single-channel BLE-band TX engine. Phase 5.2 abstracts
// "transmit a 31-byte BLE adv payload on channel 37/38/39" so the
// same operator path can drive either S3 NimBLE (channel-pinned via
// gap_adv_params.channel_map) or Hydra nRF24 (channel-mapped to
// nRF24 channel = (ch - 37) * 2 with whitening per Core spec §6.B.3).
//
// Each rail is independent — call site can fire S3 + nRF24 in
// parallel (channel-divided spam ~3x rate, or spam+jam combo).
//
// kChannel37/38/39 are the only valid channels; impls may reject
// anything else.
#include <cstdint>
#include <cstddef>

namespace yui {

class IBleRawTx {
 public:
  static constexpr int kChannel37 = 37;
  static constexpr int kChannel38 = 38;
  static constexpr int kChannel39 = 39;

  virtual ~IBleRawTx() = default;

  virtual bool        is_present() const = 0;
  virtual const char* rail_name()  const = 0;       // "NimBLE" / "nRF24"

  virtual bool set_channel(int adv_channel) = 0;    // 37 | 38 | 39
  virtual int  channel()    const         = 0;

  // One-shot adv: transmit payload (≤ 31 bytes) on the current channel.
  // Returns true on enqueue success.
  virtual bool tx_advert(const uint8_t* payload, std::size_t len) = 0;

  // Continuous TX (jam): start emitting carrier / repeated payload
  // until stop() is called. Used by BleJammerApp's "Both" rail mode.
  virtual bool start_continuous(int adv_channel) = 0;
  virtual void stop()                            = 0;
  virtual bool is_active() const                 = 0;

  // TX statistics — useful for a future HydraStatusApp readout.
  virtual std::size_t tx_count() const = 0;
};

}  // namespace yui

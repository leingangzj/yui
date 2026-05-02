#pragma once
// 802.11 frame helpers — minimum needed for v0.2 Track C apps:
// detect probe requests, extract source MAC, extract SSID from
// information elements, recognize EAPOL data frames.
//
// Frame format (MAC header):
//   FC(2) Dur(2) Addr1(6) Addr2(6) Addr3(6) SeqCtl(2) [Addr4(6)] [QoS(2)]
//
// FC byte 0:
//   bits 0-1: Protocol Version (00)
//   bits 2-3: Type (00=mgmt, 01=ctrl, 10=data, 11=ext)
//   bits 4-7: Subtype
//
// FC byte 1: ToDS, FromDS, MoreFrag, Retry, PwrMgmt, MoreData, Protected, Order
//
// SSID information element (in mgmt frame body, tagged params):
//   Tag(1) Len(1) <SSID bytes>
//   SSID tag is 0.
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace yui::dot11 {

constexpr uint8_t kTypeMgmt = 0;
constexpr uint8_t kTypeCtrl = 1;
constexpr uint8_t kTypeData = 2;

// Management subtypes
constexpr uint8_t kSubAssocReq    = 0x00;
constexpr uint8_t kSubAssocResp   = 0x01;
constexpr uint8_t kSubReassocReq  = 0x02;
constexpr uint8_t kSubReassocResp = 0x03;
constexpr uint8_t kSubProbeReq    = 0x04;
constexpr uint8_t kSubProbeResp   = 0x05;
constexpr uint8_t kSubBeacon      = 0x08;
constexpr uint8_t kSubAuth        = 0x0B;
constexpr uint8_t kSubDeauth      = 0x0C;

// Information-element tag IDs
constexpr uint8_t kIeSsid = 0x00;

inline uint8_t fc_type(const uint8_t* frame) {
  return (frame[0] >> 2) & 0x03;
}
inline uint8_t fc_subtype(const uint8_t* frame) {
  return (frame[0] >> 4) & 0x0F;
}

// Address fields. For mgmt frames:
//   Addr1 = DA (destination)
//   Addr2 = SA (source / transmitter)
//   Addr3 = BSSID
inline const uint8_t* addr1(const uint8_t* frame) { return frame + 4; }
inline const uint8_t* addr2(const uint8_t* frame) { return frame + 10; }
inline const uint8_t* addr3(const uint8_t* frame) { return frame + 16; }

// Mgmt frame body starts at offset 24 (after 24-byte MAC header).
// Beacon/probe-resp body has 12 bytes of fixed parameters (timestamp,
// beacon interval, capability) before tagged params; probe-req body is
// just tagged params (no fixed-params).
inline size_t mgmt_fixed_params_for_subtype(uint8_t subtype) {
  switch (subtype) {
    case kSubBeacon:
    case kSubProbeResp:
    case kSubAssocResp:
    case kSubReassocResp:
      return 12;            // timestamp(8) + beacon-interval(2) + capability(2)
    case kSubProbeReq:
      return 0;             // no fixed params
    case kSubAssocReq:
      return 4;             // capability(2) + listen-interval(2)
    case kSubReassocReq:
      return 10;            // capability(2) + listen-interval(2) + current-AP(6)
    default:
      return 0;
  }
}

// Find the SSID information element in a mgmt-frame body. Copies into
// out as a C string (cap chars max). Returns true on success.
inline bool extract_ssid(const uint8_t* frame, size_t len,
                         char* out, size_t cap) {
  if (!frame || !out || cap == 0) return false;
  if (len < 24) return false;
  if (fc_type(frame) != kTypeMgmt) return false;
  const uint8_t st = fc_subtype(frame);
  size_t off = 24 + mgmt_fixed_params_for_subtype(st);
  while (off + 2 <= len) {
    const uint8_t tag = frame[off];
    const uint8_t ie_len = frame[off + 1];
    if (off + 2 + ie_len > len) return false;
    if (tag == kIeSsid) {
      const size_t copy = (ie_len < cap - 1) ? ie_len : cap - 1;
      std::memcpy(out, frame + off + 2, copy);
      out[copy] = '\0';
      return true;
    }
    off += 2 + ie_len;
  }
  return false;
}

inline bool is_probe_request(const uint8_t* frame, size_t len) {
  if (!frame || len < 24) return false;
  return fc_type(frame) == kTypeMgmt && fc_subtype(frame) == kSubProbeReq;
}

inline bool is_eapol_data(const uint8_t* frame, size_t len) {
  // QoS or non-QoS data frame containing an LLC/SNAP header with
  // EAPOL ethertype 0x888E.
  if (!frame || len < 24) return false;
  if (fc_type(frame) != kTypeData) return false;
  // Skip MAC header. For non-QoS data frames it's 24 bytes; for QoS
  // (subtype 8) the header is 26 bytes.
  size_t hdr = (fc_subtype(frame) & 0x08) ? 26 : 24;
  if (len < hdr + 8) return false;
  // LLC/SNAP: AA AA 03 00 00 00 [EthType high] [EthType low]
  if (frame[hdr] != 0xAA || frame[hdr + 1] != 0xAA || frame[hdr + 2] != 0x03)
    return false;
  return frame[hdr + 6] == 0x88 && frame[hdr + 7] == 0x8E;
}

// MAC formatter: 6 bytes -> "AA:BB:CC:DD:EE:FF\0" (18 chars).
inline void format_mac(const uint8_t mac[6], char out[18]) {
  std::snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Parse "AA:BB:CC:DD:EE:FF" string into 6 bytes. Returns true on success.
inline bool parse_mac(const char* s, uint8_t out[6]) {
  if (!s) return false;
  unsigned a, b, c, d, e, f;
  if (std::sscanf(s, "%2x:%2x:%2x:%2x:%2x:%2x",
                  &a, &b, &c, &d, &e, &f) != 6) return false;
  out[0] = a; out[1] = b; out[2] = c; out[3] = d; out[4] = e; out[5] = f;
  return true;
}

// Build a beacon frame with the given SSID + BSSID + channel.
// Returns bytes written, 0 on failure. Caller-supplied buffer needs
// >= 38 + ssid_len bytes.
inline size_t build_beacon(uint8_t* out, size_t cap,
                           const char* ssid, size_t ssid_len,
                           const uint8_t bssid[6], uint8_t channel) {
  if (!out || cap < 38 + ssid_len) return 0;
  size_t off = 0;
  // FC: type=mgmt(0), subtype=beacon(8) → byte0 = 0x80
  out[off++] = 0x80; out[off++] = 0x00;
  // duration
  out[off++] = 0x00; out[off++] = 0x00;
  // Addr1 (DA) = broadcast
  for (int i = 0; i < 6; ++i) out[off++] = 0xFF;
  // Addr2 (SA) = BSSID
  for (int i = 0; i < 6; ++i) out[off++] = bssid[i];
  // Addr3 (BSSID)
  for (int i = 0; i < 6; ++i) out[off++] = bssid[i];
  // SeqCtl
  out[off++] = 0x00; out[off++] = 0x00;
  // Fixed params: timestamp(8) + beacon-interval(2) + capability(2)
  for (int i = 0; i < 8; ++i) out[off++] = 0x00;
  out[off++] = 0x64; out[off++] = 0x00;     // 100 TU beacon interval
  out[off++] = 0x21; out[off++] = 0x04;     // ESS + Short Slot
  // SSID IE
  out[off++] = 0x00; out[off++] = static_cast<uint8_t>(ssid_len);
  for (size_t i = 0; i < ssid_len; ++i)
    out[off++] = static_cast<uint8_t>(ssid[i]);
  // Supported Rates IE (1, 2, 5.5, 11 Mbps in 0.5 Mbps units, basic-flagged)
  out[off++] = 0x01; out[off++] = 0x04;
  out[off++] = 0x82; out[off++] = 0x84; out[off++] = 0x8B; out[off++] = 0x96;
  // DS Parameter Set: channel
  out[off++] = 0x03; out[off++] = 0x01; out[off++] = channel;
  return off;
}

// Build a deauth frame. By default sends from BSSID to DA (broadcast or
// specific client).
inline size_t build_deauth(uint8_t* out, size_t cap,
                           const uint8_t da[6], const uint8_t bssid[6],
                           uint16_t reason = 7 /* class3 from non-assoc STA */) {
  if (!out || cap < 26) return 0;
  size_t off = 0;
  // FC: type=mgmt(0), subtype=deauth(12) → byte0 = 0xC0
  out[off++] = 0xC0; out[off++] = 0x00;
  out[off++] = 0x00; out[off++] = 0x00;
  for (int i = 0; i < 6; ++i) out[off++] = da[i];
  for (int i = 0; i < 6; ++i) out[off++] = bssid[i];
  for (int i = 0; i < 6; ++i) out[off++] = bssid[i];
  out[off++] = 0x00; out[off++] = 0x00;
  // Reason code (LE)
  out[off++] = static_cast<uint8_t>(reason & 0xFF);
  out[off++] = static_cast<uint8_t>((reason >> 8) & 0xFF);
  return off;
}

// Detect WPS support in a beacon: vendor-specific IE (0xDD) with
// Microsoft OUI 00:50:F2 type 0x04.
inline bool has_wps_ie(const uint8_t* frame, size_t len) {
  if (!frame || len < 24) return false;
  if (fc_type(frame) != kTypeMgmt) return false;
  const uint8_t st = fc_subtype(frame);
  if (st != kSubBeacon && st != kSubProbeResp) return false;
  size_t off = 24 + mgmt_fixed_params_for_subtype(st);
  while (off + 2 <= len) {
    const uint8_t tag    = frame[off];
    const uint8_t ie_len = frame[off + 1];
    if (off + 2 + ie_len > len) return false;
    if (tag == 0xDD && ie_len >= 4) {
      const uint8_t* p = frame + off + 2;
      if (p[0] == 0x00 && p[1] == 0x50 && p[2] == 0xF2 && p[3] == 0x04)
        return true;
    }
    off += 2 + ie_len;
  }
  return false;
}

}  // namespace yui::dot11

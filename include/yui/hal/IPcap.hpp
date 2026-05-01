#pragma once
// IPcap — write libpcap-format capture files for analysis with Wireshark
// / aircrack-ng / hashcat. v0.2 emits DLT_IEEE802_11 (linktype 105),
// little-endian. See docs/protocols/ESP_WIFI_AND_PCAP.md §3.
#include <cstdint>
#include <cstddef>

namespace yui {

enum class PcapLinkType : uint32_t {
  Ieee80211       = 105,   // raw 802.11 frame, no metadata header
  Ieee80211Radio  = 127,   // 802.11 with Radiotap header (v0.3+)
};

class IPcap {
public:
  virtual ~IPcap() = default;

  // Open a new file at path; writes the 24-byte global header.
  // Closes any previously-open file. snaplen is max bytes per packet.
  virtual bool open(const char* path,
                    PcapLinkType linktype = PcapLinkType::Ieee80211,
                    uint32_t snaplen = 65535) = 0;

  // Append one packet record (16-byte record header + payload).
  // ts_us is microseconds since Unix epoch.
  virtual bool write_packet(const uint8_t* frame,
                            size_t frame_len,
                            uint64_t ts_us) = 0;

  // Flush + close. Safe to call multiple times.
  virtual void close() = 0;

  // Total bytes written (incl. headers) since open(). Resets on each open.
  virtual uint64_t bytes_written() const = 0;
};

}  // namespace yui

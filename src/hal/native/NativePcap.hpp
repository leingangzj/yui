#pragma once
#include "yui/hal/IPcap.hpp"
#include <cstring>
#include <string>
#include <vector>

namespace yui {

// FakePcap — backs onto an in-memory buffer instead of a real file, so
// tests can byte-compare the output to a known-good golden capture.
// The on-disk format is identical to libpcap (LE header, 16-byte
// per-record), so this fake is also a working reference impl.
class FakePcap : public IPcap {
public:
  bool open(const char* path, PcapLinkType linktype, uint32_t snaplen) override {
    path_   = path ? path : "";
    buffer_.clear();
    bytes_  = 0;
    is_open_ = true;
    return write_global_header_(linktype, snaplen);
  }

  bool write_packet(const uint8_t* frame, size_t frame_len, uint64_t ts_us) override {
    if (!is_open_ || frame == nullptr) return false;
    const uint32_t ts_sec  = static_cast<uint32_t>(ts_us / 1'000'000ULL);
    const uint32_t ts_usec = static_cast<uint32_t>(ts_us % 1'000'000ULL);
    const uint32_t inc_len = static_cast<uint32_t>(frame_len);
    const uint32_t orig    = static_cast<uint32_t>(frame_len);
    write_le32_(ts_sec);
    write_le32_(ts_usec);
    write_le32_(inc_len);
    write_le32_(orig);
    buffer_.insert(buffer_.end(), frame, frame + frame_len);
    bytes_ += 16 + frame_len;
    return true;
  }

  void close() override { is_open_ = false; }
  uint64_t bytes_written() const override { return bytes_; }

  // Test inspection
  const std::vector<uint8_t>& buffer() const { return buffer_; }
  const std::string& path() const { return path_; }
  bool is_open() const { return is_open_; }

private:
  bool write_global_header_(PcapLinkType lt, uint32_t snaplen) {
    write_le32_(0xA1B2C3D4u);   // magic — we always emit BE-order magic
                                // and let the file be read either way;
                                // canonical libpcap reads both.
    write_le16_(0x0002);        // version major
    write_le16_(0x0004);        // version minor
    write_le32_(0u);            // thiszone (GMT)
    write_le32_(0u);            // sigfigs
    write_le32_(snaplen);
    write_le32_(static_cast<uint32_t>(lt));
    bytes_ = 24;
    return true;
  }
  void write_le16_(uint16_t v) {
    buffer_.push_back(v & 0xFF);
    buffer_.push_back((v >> 8) & 0xFF);
  }
  void write_le32_(uint32_t v) {
    buffer_.push_back(v & 0xFF);
    buffer_.push_back((v >> 8) & 0xFF);
    buffer_.push_back((v >> 16) & 0xFF);
    buffer_.push_back((v >> 24) & 0xFF);
  }

  std::string path_;
  std::vector<uint8_t> buffer_;
  uint64_t bytes_ = 0;
  bool is_open_  = false;
};

}  // namespace yui

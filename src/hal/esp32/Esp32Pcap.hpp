#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32Pcap — libpcap streamer to SD via Arduino's SD library.
// Uses fsync every kFlushIntervalBytes so a power-cycle loses at
// most a few seconds of capture.
#include "yui/hal/IPcap.hpp"
#include <SD.h>
#include <FS.h>

namespace yui {

class Esp32Pcap : public IPcap {
public:
  static constexpr uint64_t kFlushInterval = 16 * 1024;

  bool open(const char* path, PcapLinkType linktype, uint32_t snaplen) override {
    close();
    if (!path) return false;
    if (!SD.begin()) return false;
    file_ = SD.open(path, FILE_WRITE);
    if (!file_) return false;
    bytes_       = 0;
    since_flush_ = 0;
    write_global_header_(linktype, snaplen);
    return true;
  }

  bool write_packet(const uint8_t* frame, size_t frame_len, uint64_t ts_us) override {
    if (!file_ || !frame) return false;
    const uint32_t ts_sec  = static_cast<uint32_t>(ts_us / 1'000'000ULL);
    const uint32_t ts_usec = static_cast<uint32_t>(ts_us % 1'000'000ULL);
    const uint32_t inc_len = static_cast<uint32_t>(frame_len);
    write_le32_(ts_sec);
    write_le32_(ts_usec);
    write_le32_(inc_len);
    write_le32_(inc_len);
    file_.write(frame, frame_len);
    bytes_       += 16 + frame_len;
    since_flush_ += 16 + frame_len;
    if (since_flush_ >= kFlushInterval) {
      file_.flush();
      since_flush_ = 0;
    }
    return true;
  }

  void close() override {
    if (file_) { file_.flush(); file_.close(); }
  }

  uint64_t bytes_written() const override { return bytes_; }

private:
  void write_global_header_(PcapLinkType lt, uint32_t snaplen) {
    write_le32_(0xA1B2C3D4u);
    write_le16_(2); write_le16_(4);
    write_le32_(0); write_le32_(0);
    write_le32_(snaplen);
    write_le32_(static_cast<uint32_t>(lt));
    bytes_ = 24;
  }
  void write_le16_(uint16_t v) {
    uint8_t b[2] = { static_cast<uint8_t>(v & 0xFF),
                     static_cast<uint8_t>((v >> 8) & 0xFF) };
    file_.write(b, 2);
  }
  void write_le32_(uint32_t v) {
    uint8_t b[4] = { static_cast<uint8_t>(v & 0xFF),
                     static_cast<uint8_t>((v >> 8) & 0xFF),
                     static_cast<uint8_t>((v >> 16) & 0xFF),
                     static_cast<uint8_t>((v >> 24) & 0xFF) };
    file_.write(b, 4);
  }

  File     file_;
  uint64_t bytes_       = 0;
  uint64_t since_flush_ = 0;
};

}  // namespace yui
#endif

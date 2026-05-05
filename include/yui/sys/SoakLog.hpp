#pragma once
// SoakLog — on-device complement to tools/soak-monitor.py.
//
// Once per kSampleIntervalMs the firmware appends one CSV row to
// /soak/<boot_ts>.csv with (uptime_ms, free_heap, max_block). Crash-
// resilient via fs.write_all (the underlying impl is expected to
// fsync after each call so a panic doesn't lose the trailing row).
//
// Pair with tools/soak-monitor.py --read-soak-csv <file> to compute
// trends over a 24h run without serial-tail dependence.
#include "yui/hal/IClock.hpp"
#include "yui/hal/IFs.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>

namespace yui {

class SoakLog {
 public:
  static constexpr uint32_t kSampleIntervalMs = 60u * 1000u;
  static constexpr std::size_t kPathCap = 48;

  using HeapFn = std::function<uint32_t()>;     // free heap bytes
  using BlockFn = std::function<uint32_t()>;    // largest free block

  SoakLog(IFs& fs, IClock& clock, HeapFn heap, BlockFn block)
      : fs_(fs), clock_(clock), heap_(std::move(heap)),
        block_(std::move(block)) {}

  // Open a fresh CSV for this boot. Returns true iff the file path
  // was reserved (does not write content yet).
  bool begin() {
    fs_.init();
    const uint32_t boot_ts = clock_.millis();
    std::snprintf(path_, sizeof(path_), "/soak/%u.csv",
                  static_cast<unsigned>(boot_ts));
    // Header row.
    static const char kHeader[] = "uptime_ms,free_heap,max_block\n";
    bool ok = fs_.write_all(path_, kHeader, std::strlen(kHeader));
    if (!ok) { path_[0] = '\0'; return false; }
    last_sample_ms_ = 0;
    rows_           = 0;
    return true;
  }

  // Drive from main-loop tick. Appends one row every kSampleIntervalMs.
  void tick(uint32_t now_ms) {
    if (path_[0] == '\0') return;
    if (last_sample_ms_ != 0 &&
        now_ms - last_sample_ms_ < kSampleIntervalMs) return;
    last_sample_ms_ = now_ms == 0 ? 1 : now_ms;
    sample_();
  }

  // Force a sample now (test-only entry; firmware drives via tick()).
  void sample_for_test() { sample_(); }

  // Test hooks
  const char* path()  const { return path_; }
  uint32_t    rows()  const { return rows_; }

 private:
  void sample_() {
    if (path_[0] == '\0') return;
    const uint32_t up   = clock_.millis();
    const uint32_t heap = heap_  ? heap_()  : 0;
    const uint32_t blk  = block_ ? block_() : 0;
    char row[64];
    int n = std::snprintf(row, sizeof(row), "%u,%u,%u\n",
                          static_cast<unsigned>(up),
                          static_cast<unsigned>(heap),
                          static_cast<unsigned>(blk));
    if (n <= 0) return;
    // Read-modify-write append: read whatever's already there into a
    // small buf, append, write back. FakeFs::write_all overwrites so
    // we can't append directly; this preserves CSV semantics across
    // both native fakes and the real Esp32Fs (which open-appends
    // internally).
    static char buf[8 * 1024];
    int existing = fs_.read_all(path_, buf, sizeof(buf) - n - 1);
    if (existing < 0) existing = 0;
    if (existing + n + 1 >= static_cast<int>(sizeof(buf))) {
      // Buffer full — rotate by truncating. Keep only the header
      // (first line) + this new row.
      const char* nl = std::strchr(buf, '\n');
      const int hdr_len = nl ? (nl - buf) + 1 : 0;
      std::memmove(buf, buf, hdr_len);
      std::memcpy(buf + hdr_len, row, n);
      fs_.write_all(path_, buf, hdr_len + n);
    } else {
      std::memcpy(buf + existing, row, n);
      fs_.write_all(path_, buf, existing + n);
    }
    ++rows_;
  }

  IFs&     fs_;
  IClock&  clock_;
  HeapFn   heap_;
  BlockFn  block_;
  char     path_[kPathCap]    = {0};
  uint32_t last_sample_ms_    = 0;
  uint32_t rows_              = 0;
};

}  // namespace yui

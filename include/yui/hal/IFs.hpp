#pragma once
#include <cstddef>
#include <cstdint>

namespace yui {

struct FsEntry {
  char     name[64] = {0};
  bool     is_dir   = false;
  uint32_t size     = 0;
};

class IFs {
public:
  virtual ~IFs() = default;

  virtual bool init() = 0;
  virtual bool exists(const char* path) = 0;

  // Read up to (cap-1) bytes into out_buf, NUL-terminate, return bytes read.
  // Returns -1 on error.
  virtual int read_all(const char* path, char* out_buf, size_t cap) = 0;

  // Write `len` bytes; returns true on success.
  virtual bool write_all(const char* path, const char* data, size_t len) = 0;

  // List entries in `dir` into out (capacity max). Returns count written or -1.
  virtual int list(const char* dir, FsEntry* out, size_t max) = 0;
};

}  // namespace yui

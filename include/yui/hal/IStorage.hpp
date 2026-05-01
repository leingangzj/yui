#pragma once
// Persistent key-value storage. On the ESP32, backed by NVS (~3 KB usable
// per-namespace). On native, backed by an in-memory map.
#include <cstdint>
#include <cstddef>

namespace yui {

class IStorage {
public:
  virtual ~IStorage() = default;
  virtual bool init() = 0;
  virtual bool put_int(const char* key, int32_t value) = 0;
  virtual bool get_int(const char* key, int32_t& out, int32_t def = 0) = 0;
  virtual bool put_str(const char* key, const char* value) = 0;
  // Reads up to (cap-1) bytes into out, NUL-terminates. Returns true on hit.
  virtual bool get_str(const char* key, char* out, size_t cap) = 0;
  virtual bool erase(const char* key) = 0;
};

}  // namespace yui

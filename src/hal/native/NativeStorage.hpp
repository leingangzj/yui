#pragma once
#include "yui/hal/IStorage.hpp"
#include <map>
#include <string>
#include <cstring>

namespace yui {

class FakeStorage : public IStorage {
public:
  bool init() override { return true; }

  bool put_int(const char* key, int32_t v) override {
    ints_[key] = v; return true;
  }
  bool get_int(const char* key, int32_t& out, int32_t def) override {
    auto it = ints_.find(key);
    if (it == ints_.end()) { out = def; return false; }
    out = it->second; return true;
  }
  bool put_str(const char* key, const char* v) override {
    strs_[key] = v ? v : ""; return true;
  }
  bool get_str(const char* key, char* out, size_t cap) override {
    if (cap == 0) return false;
    auto it = strs_.find(key);
    if (it == strs_.end()) { out[0] = '\0'; return false; }
    const size_t n = std::min(cap - 1, it->second.size());
    std::memcpy(out, it->second.data(), n);
    out[n] = '\0';
    return true;
  }
  bool erase(const char* key) override {
    ints_.erase(key); strs_.erase(key); return true;
  }

private:
  std::map<std::string, int32_t>     ints_;
  std::map<std::string, std::string> strs_;
};

}  // namespace yui

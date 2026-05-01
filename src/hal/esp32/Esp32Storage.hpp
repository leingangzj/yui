#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IStorage.hpp"
#include <Preferences.h>
#include <cstring>

namespace yui {

class Esp32Storage : public IStorage {
public:
  static constexpr const char* kNamespace = "yui";

  bool init() override {
    if (inited_) return true;
    inited_ = prefs_.begin(kNamespace, /*readOnly=*/false);
    return inited_;
  }

  bool put_int(const char* key, int32_t v) override {
    if (!init()) return false;
    return prefs_.putInt(key, v) > 0;
  }
  bool get_int(const char* key, int32_t& out, int32_t def) override {
    if (!init()) { out = def; return false; }
    out = prefs_.getInt(key, def);
    return prefs_.isKey(key);
  }
  bool put_str(const char* key, const char* v) override {
    if (!init() || v == nullptr) return false;
    return prefs_.putString(key, v) > 0;
  }
  bool get_str(const char* key, char* out, size_t cap) override {
    if (!init() || cap == 0) return false;
    const String s = prefs_.getString(key, "");
    const size_t n = std::min(cap - 1, static_cast<size_t>(s.length()));
    std::memcpy(out, s.c_str(), n);
    out[n] = '\0';
    return s.length() > 0;
  }
  bool erase(const char* key) override {
    if (!init()) return false;
    return prefs_.remove(key);
  }

private:
  Preferences prefs_;
  bool        inited_ = false;
};

}  // namespace yui
#endif

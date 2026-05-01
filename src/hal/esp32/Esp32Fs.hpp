#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IFs.hpp"
#include <SD.h>
#include <FS.h>
#include <cstring>

namespace yui {

class Esp32Fs : public IFs {
public:
  bool init() override {
    if (inited_) return true;
    // CS pin TBD — M5Unified may have already initialized SD via M5.begin().
    // For v0.0 we rely on Arduino's SD.begin() default; if it fails, the
    // Files app will surface the error in its UI.
    inited_ = SD.begin();
    return inited_;
  }

  bool exists(const char* path) override {
    if (!init()) return false;
    return SD.exists(path);
  }

  int read_all(const char* path, char* out_buf, size_t cap) override {
    if (!init() || cap == 0) return -1;
    File f = SD.open(path, FILE_READ);
    if (!f) return -1;
    const size_t n = std::min<size_t>(cap - 1, f.size());
    f.read(reinterpret_cast<uint8_t*>(out_buf), n);
    out_buf[n] = '\0';
    f.close();
    return static_cast<int>(n);
  }

  bool write_all(const char* path, const char* data, size_t len) override {
    if (!init()) return false;
    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;
    const size_t w = f.write(reinterpret_cast<const uint8_t*>(data), len);
    f.close();
    return w == len;
  }

  int list(const char* dir, FsEntry* out, size_t max) override {
    if (!init()) return -1;
    File d = SD.open(dir);
    if (!d || !d.isDirectory()) return -1;
    int n = 0;
    while (n < static_cast<int>(max)) {
      File e = d.openNextFile();
      if (!e) break;
      const char* name = e.name();
      // Strip leading directory if SD returns full paths.
      const char* slash = std::strrchr(name, '/');
      if (slash) name = slash + 1;
      std::strncpy(out[n].name, name, sizeof(out[n].name) - 1);
      out[n].is_dir = e.isDirectory();
      out[n].size   = e.size();
      e.close();
      ++n;
    }
    d.close();
    return n;
  }

private:
  bool inited_ = false;
};

}  // namespace yui
#endif

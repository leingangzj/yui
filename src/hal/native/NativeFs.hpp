#pragma once
#include "yui/hal/IFs.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

namespace yui {

// In-memory filesystem for tests. Path semantics are simplistic:
//  - "/" is root
//  - paths are split on '/'; last segment is the entry name
//  - exists() checks both files and directories; init creates "/"
class FakeFs : public IFs {
public:
  bool init() override {
    if (!inited_) ensure_dir_("/");
    inited_ = true;
    return true;
  }

  bool exists(const char* path) override {
    return files_.count(path) > 0 || dirs_.count(path) > 0;
  }

  int read_all(const char* path, char* out_buf, size_t cap) override {
    auto it = files_.find(path);
    if (it == files_.end() || cap == 0) return -1;
    const size_t n = std::min(cap - 1, it->second.size());
    std::memcpy(out_buf, it->second.data(), n);
    out_buf[n] = '\0';
    return static_cast<int>(n);
  }

  bool write_all(const char* path, const char* data, size_t len) override {
    files_[path] = std::string(data, data + len);
    ensure_parent_dir_(path);
    return true;
  }

  int list(const char* dir, FsEntry* out, size_t max) override {
    if (dirs_.count(dir) == 0) return -1;
    std::string prefix = dir;
    if (prefix.back() != '/') prefix += "/";
    std::vector<FsEntry> hits;
    for (const auto& kv : files_) collect_(kv.first, prefix, false, kv.second.size(), hits);
    for (const auto& d : dirs_)   collect_(d,        prefix, true,  0,                 hits);
    std::sort(hits.begin(), hits.end(),
              [](const FsEntry& a, const FsEntry& b) {
                return std::strcmp(a.name, b.name) < 0;
              });
    const size_t n = std::min(max, hits.size());
    for (size_t i = 0; i < n; ++i) out[i] = hits[i];
    return static_cast<int>(n);
  }

  // Test helpers
  void put_file(const char* path, const std::string& data) { write_all(path, data.data(), data.size()); }
  void mkdir(const char* path) { ensure_dir_(path); }

private:
  void ensure_dir_(const std::string& path) { dirs_.insert(path); }
  void ensure_parent_dir_(const std::string& path) {
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos || pos == 0) { ensure_dir_("/"); return; }
    ensure_dir_(path.substr(0, pos));
  }
  static void collect_(const std::string& full, const std::string& prefix,
                       bool is_dir, uint32_t size, std::vector<FsEntry>& out) {
    if (full.size() <= prefix.size() ||
        full.compare(0, prefix.size(), prefix) != 0) return;
    const std::string rest = full.substr(prefix.size());
    if (rest.find('/') != std::string::npos) return;  // deeper, skip
    FsEntry e;
    std::strncpy(e.name, rest.c_str(), sizeof(e.name) - 1);
    e.is_dir = is_dir;
    e.size   = size;
    out.push_back(e);
  }

  std::map<std::string, std::string> files_;
  std::set<std::string> dirs_;
  bool inited_ = false;
};

}  // namespace yui

#pragma once
#include "yui/app/App.hpp"
#include <array>
#include <cstddef>

namespace yui {

// Fixed-capacity registry of apps. No heap, no globals beyond a single
// per-Shell instance. Apps are registered at startup before the Shell runs.
class AppRegistry {
public:
  static constexpr std::size_t kMaxApps = 40;

  bool add(App* app) {
    if (count_ >= kMaxApps || app == nullptr) return false;
    apps_[count_++] = app;
    return true;
  }

  std::size_t size() const { return count_; }
  App* at(std::size_t i) const { return (i < count_) ? apps_[i] : nullptr; }

private:
  std::array<App*, kMaxApps> apps_{};
  std::size_t count_ = 0;
};

}  // namespace yui

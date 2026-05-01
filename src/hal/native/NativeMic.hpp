#pragma once
#include "yui/hal/IMic.hpp"
#include <vector>
#include <cstring>
#include <algorithm>

namespace yui {

class FakeMic : public IMic {
public:
  bool init() override { return true; }

  size_t read_samples(int16_t* out, size_t cap) override {
    const size_t n = std::min(cap, queued_.size());
    if (n == 0) return 0;
    std::memcpy(out, queued_.data(), n * sizeof(int16_t));
    queued_.erase(queued_.begin(), queued_.begin() + n);
    return n;
  }

  void push(const int16_t* s, size_t n) { queued_.insert(queued_.end(), s, s + n); }
  void push_constant(int16_t v, size_t n) { queued_.insert(queued_.end(), n, v); }

private:
  std::vector<int16_t> queued_;
};

}  // namespace yui

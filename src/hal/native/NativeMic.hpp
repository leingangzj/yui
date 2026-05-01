#pragma once
#include "yui/hal/IMic.hpp"
#include <vector>
#include <cstring>
#include <algorithm>
#include <cmath>

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
  void push_sine(float freq_hz, float sample_rate, int16_t amplitude, size_t n) {
    constexpr float kPi = 3.14159265358979323846f;
    queued_.reserve(queued_.size() + n);
    for (size_t i = 0; i < n; ++i) {
      const float t = static_cast<float>(i) / sample_rate;
      queued_.push_back(static_cast<int16_t>(amplitude * std::sin(2.f * kPi * freq_hz * t)));
    }
  }

private:
  std::vector<int16_t> queued_;
};

}  // namespace yui

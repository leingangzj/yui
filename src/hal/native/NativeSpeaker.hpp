#pragma once
#include "yui/hal/ISpeaker.hpp"
#include <vector>

namespace yui {

class FakeSpeaker : public ISpeaker {
public:
  struct Tone { uint32_t freq_hz; uint32_t duration_ms; };

  bool init() override { ++init_count_; return true; }
  void tone(uint32_t freq_hz, uint32_t duration_ms) override {
    history_.push_back({freq_hz, duration_ms});
  }
  void stop() override { ++stop_count_; }

  // Test hooks
  size_t       count() const { return history_.size(); }
  const Tone&  at(size_t i) const { return history_[i]; }
  Tone         last() const { return history_.empty() ? Tone{0, 0} : history_.back(); }
  int          init_count() const { return init_count_; }
  int          stop_count() const { return stop_count_; }

private:
  std::vector<Tone> history_;
  int init_count_ = 0;
  int stop_count_ = 0;
};

}  // namespace yui

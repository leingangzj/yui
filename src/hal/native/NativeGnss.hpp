#pragma once
#include "yui/hal/IGnss.hpp"

namespace yui {

class FakeGnss : public IGnss {
public:
  GnssFix latest() override { return fix_; }
  bool any_data_seen() const override { return seen_; }

  // Test fixture
  void set_fix(const GnssFix& f) { fix_ = f; seen_ = true; }
  void mark_seen() { seen_ = true; }
  void reset()    { fix_ = {}; seen_ = false; }

private:
  GnssFix fix_{};
  bool    seen_ = false;
};

}  // namespace yui

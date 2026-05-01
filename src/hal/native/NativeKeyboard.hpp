#pragma once
#include "yui/hal/IKeyboard.hpp"
#include <queue>

namespace yui {

// Test-driver keyboard: tests push events; firmware polls them.
class MockKeyboard : public IKeyboard {
public:
  bool poll(KeyEvent& out) override {
    if (q_.empty()) return false;
    out = q_.front();
    q_.pop();
    return true;
  }
  void inject(const KeyEvent& e) { q_.push(e); }
  void inject_press(Key k) {
    KeyEvent e{};
    e.key  = k;
    e.down = true;
    q_.push(e);
  }

private:
  std::queue<KeyEvent> q_;
};

}  // namespace yui

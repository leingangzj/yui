#pragma once
#include <cstddef>

namespace yui {

// Pure-logic menu cursor: knows nothing about display or input.
// All HAL-free → fully testable in native env.
class Menu {
public:
  Menu(std::size_t item_count) : count_(item_count), cursor_(0) {}

  void up()   { cursor_ = (cursor_ == 0) ? count_ - 1 : cursor_ - 1; }
  void down() { cursor_ = (cursor_ + 1) % count_; }

  std::size_t cursor() const { return cursor_; }
  std::size_t count()  const { return count_; }

  void set_count(std::size_t n) {
    count_ = n;
    if (cursor_ >= count_ && count_ > 0) cursor_ = count_ - 1;
    if (count_ == 0) cursor_ = 0;
  }

  void set_cursor(std::size_t c) {
    if (count_ == 0) { cursor_ = 0; return; }
    cursor_ = (c >= count_) ? count_ - 1 : c;
  }

private:
  std::size_t count_;
  std::size_t cursor_;
};

}  // namespace yui

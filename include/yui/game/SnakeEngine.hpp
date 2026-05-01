#pragma once
// Pure-logic snake game engine. No display, no input — just grid state +
// step(). The host App is responsible for ticking and rendering.
#include <cstddef>
#include <cstdint>

namespace yui {

class SnakeEngine {
public:
  static constexpr int    kCols     = 30;
  static constexpr int    kRows     = 11;
  static constexpr size_t kMaxBody  = kCols * kRows;

  enum class Dir   { Up, Down, Left, Right };
  enum class State { Idle, Running, GameOver };

  struct Cell { int x; int y; };

  SnakeEngine() { reset(1); }

  void reset(uint32_t seed) {
    rng_     = seed ? seed : 1u;
    state_   = State::Running;
    dir_     = Dir::Right;
    pending_ = Dir::Right;
    len_     = 3;
    const int cx = kCols / 2;
    const int cy = kRows / 2;
    body_[0] = {cx,     cy};   // head
    body_[1] = {cx - 1, cy};
    body_[2] = {cx - 2, cy};
    place_food_();
    score_ = 0;
  }

  // Queue a turn. Reverses are ignored — you can't U-turn into yourself.
  void turn(Dir d) {
    if (state_ != State::Running) return;
    if ((dir_ == Dir::Up    && d == Dir::Down)  ||
        (dir_ == Dir::Down  && d == Dir::Up)    ||
        (dir_ == Dir::Left  && d == Dir::Right) ||
        (dir_ == Dir::Right && d == Dir::Left)) return;
    pending_ = d;
  }

  void step() {
    if (state_ != State::Running) return;
    dir_ = pending_;
    Cell next = body_[0];
    switch (dir_) {
      case Dir::Up:    --next.y; break;
      case Dir::Down:  ++next.y; break;
      case Dir::Left:  --next.x; break;
      case Dir::Right: ++next.x; break;
    }
    // Wall collision.
    if (next.x < 0 || next.x >= kCols || next.y < 0 || next.y >= kRows) {
      state_ = State::GameOver;
      return;
    }
    const bool ate = (next.x == food_.x && next.y == food_.y);
    const size_t check_len = ate ? len_ : len_ - 1;  // tail clears unless eating
    for (size_t i = 0; i < check_len; ++i) {
      if (body_[i].x == next.x && body_[i].y == next.y) {
        state_ = State::GameOver;
        return;
      }
    }
    // Shift body. If eating, grow by one.
    const size_t new_len = ate ? len_ + 1 : len_;
    if (new_len > kMaxBody) { state_ = State::GameOver; return; }
    for (size_t i = new_len - 1; i > 0; --i) body_[i] = body_[i - 1];
    body_[0] = next;
    len_     = new_len;
    if (ate) {
      ++score_;
      place_food_();
    }
  }

  State  state()  const { return state_; }
  Dir    dir()    const { return dir_; }
  size_t length() const { return len_; }
  Cell   head()   const { return body_[0]; }
  Cell   at(size_t i) const { return body_[i]; }
  Cell   food()   const { return food_; }
  int    score()  const { return score_; }

  bool occupies(int x, int y) const {
    for (size_t i = 0; i < len_; ++i)
      if (body_[i].x == x && body_[i].y == y) return true;
    return false;
  }

private:
  uint32_t lcg_() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return rng_;
  }
  void place_food_() {
    // Pick random empty cell. Bounded loop: at worst sweep all cells.
    for (int tries = 0; tries < kCols * kRows * 4; ++tries) {
      const int x = static_cast<int>(lcg_() % kCols);
      const int y = static_cast<int>(lcg_() % kRows);
      if (!occupies(x, y)) { food_ = {x, y}; return; }
    }
    // Fallback: linear scan.
    for (int y = 0; y < kRows; ++y)
      for (int x = 0; x < kCols; ++x)
        if (!occupies(x, y)) { food_ = {x, y}; return; }
    state_ = State::GameOver;  // grid full = win, but mark done
  }

  Cell     body_[kMaxBody]{};
  size_t   len_     = 0;
  Dir      dir_     = Dir::Right;
  Dir      pending_ = Dir::Right;
  Cell     food_    = {0, 0};
  State    state_   = State::Idle;
  int      score_   = 0;
  uint32_t rng_     = 1;
};

}  // namespace yui

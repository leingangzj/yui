#pragma once
// Conway's Game of Life on a fixed-size grid. No display, no input.
// Toroidal wrap so gliders stay alive — keeps the visual interesting on a
// small screen.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace yui {

class LifeEngine {
public:
  static constexpr int kCols = 60;
  static constexpr int kRows = 24;

  enum class Seed { Empty, Random, Glider, Blinker, Pulsar };

  LifeEngine() { clear(); }

  void clear() {
    std::memset(cells_, 0, sizeof(cells_));
    generation_ = 0;
    population_ = 0;
  }

  void seed(Seed s, uint32_t rng_seed = 1) {
    clear();
    rng_ = rng_seed ? rng_seed : 1u;
    switch (s) {
      case Seed::Empty: break;
      case Seed::Random: random_fill_(); break;
      case Seed::Glider: place_glider_(2, 2); break;
      case Seed::Blinker: place_blinker_(kCols / 2, kRows / 2); break;
      case Seed::Pulsar: place_pulsar_(kCols / 2 - 6, kRows / 2 - 6); break;
    }
    recount_();
  }

  bool alive(int x, int y) const {
    return cells_[wrap_y_(y)][wrap_x_(x)] != 0;
  }
  void set(int x, int y, bool v) {
    cells_[wrap_y_(y)][wrap_x_(x)] = v ? 1 : 0;
  }

  void step() {
    uint8_t next[kRows][kCols];
    size_t  pop = 0;
    for (int y = 0; y < kRows; ++y) {
      for (int x = 0; x < kCols; ++x) {
        const int n = neighbor_count_(x, y);
        const bool was = cells_[y][x] != 0;
        const bool now = was ? (n == 2 || n == 3) : (n == 3);
        next[y][x] = now ? 1 : 0;
        if (now) ++pop;
      }
    }
    std::memcpy(cells_, next, sizeof(cells_));
    ++generation_;
    population_ = pop;
  }

  uint32_t generation() const { return generation_; }
  size_t   population() const { return population_; }

private:
  int wrap_x_(int x) const { return ((x % kCols) + kCols) % kCols; }
  int wrap_y_(int y) const { return ((y % kRows) + kRows) % kRows; }

  int neighbor_count_(int x, int y) const {
    int n = 0;
    for (int dy = -1; dy <= 1; ++dy)
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0) continue;
        if (cells_[wrap_y_(y + dy)][wrap_x_(x + dx)]) ++n;
      }
    return n;
  }

  uint32_t lcg_() { rng_ = rng_ * 1664525u + 1013904223u; return rng_; }
  void random_fill_() {
    for (int y = 0; y < kRows; ++y)
      for (int x = 0; x < kCols; ++x)
        cells_[y][x] = (lcg_() & 7u) < 2u ? 1 : 0;  // ~25% density
  }
  void place_glider_(int cx, int cy) {
    set(cx + 1, cy,     true);
    set(cx + 2, cy + 1, true);
    set(cx,     cy + 2, true);
    set(cx + 1, cy + 2, true);
    set(cx + 2, cy + 2, true);
  }
  void place_blinker_(int cx, int cy) {
    set(cx - 1, cy, true);
    set(cx,     cy, true);
    set(cx + 1, cy, true);
  }
  void place_pulsar_(int cx, int cy) {
    static const int kCells[][2] = {
      {2,0},{3,0},{4,0},{8,0},{9,0},{10,0},
      {0,2},{5,2},{7,2},{12,2},
      {0,3},{5,3},{7,3},{12,3},
      {0,4},{5,4},{7,4},{12,4},
      {2,5},{3,5},{4,5},{8,5},{9,5},{10,5},
      {2,7},{3,7},{4,7},{8,7},{9,7},{10,7},
      {0,8},{5,8},{7,8},{12,8},
      {0,9},{5,9},{7,9},{12,9},
      {0,10},{5,10},{7,10},{12,10},
      {2,12},{3,12},{4,12},{8,12},{9,12},{10,12},
    };
    for (const auto& c : kCells) set(cx + c[0], cy + c[1], true);
  }
  void recount_() {
    population_ = 0;
    for (int y = 0; y < kRows; ++y)
      for (int x = 0; x < kCols; ++x)
        if (cells_[y][x]) ++population_;
  }

  uint8_t  cells_[kRows][kCols] = {};
  uint32_t rng_        = 1;
  uint32_t generation_ = 0;
  size_t   population_ = 0;
};

}  // namespace yui

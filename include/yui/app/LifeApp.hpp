#pragma once
// Conway's Game of Life. Enter pauses, Tab cycles seed, Backspace clears,
// Right single-steps, Up/Down adjust speed.
#include "yui/app/App.hpp"
#include "yui/game/LifeEngine.hpp"
#include "yui/types.hpp"
#include <cstdio>

#include "yui/ui/Chrome.hpp"
#include "yui/ui/Tokens.hpp"
namespace yui {

class LifeApp : public App {
public:
  static constexpr uint32_t kStepMin   = 60;
  static constexpr uint32_t kStepMax   = 600;
  static constexpr uint32_t kStepDelta = 60;

  static constexpr int kCellPx = 4;
  static constexpr int kBoardX = 0;
  static constexpr int kBoardY = 18;

  const char* name() const override { return "Life"; }
  Category    category() const override { return Category::Fun; }

  void on_enter(Hal& hal) override {
    paused_   = false;
    last_ms_  = 0;
    next_ms_  = 0;
    step_ms_  = 200;
    seed_idx_ = 1;  // Random
    engine_.seed(LifeEngine::Seed::Random,
                 static_cast<uint32_t>(hal.clock.millis()) | 1u);
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    switch (k.key) {
      case Key::Enter:
        paused_  = !paused_;
        next_ms_ = 0;
        break;
      case Key::Backspace:
        engine_.clear();
        paused_ = true;
        break;
      case Key::Right:
        engine_.step();
        break;
      case Key::Up:
        if (step_ms_ > kStepMin) step_ms_ -= kStepDelta;
        break;
      case Key::Down:
        if (step_ms_ < kStepMax) step_ms_ += kStepDelta;
        break;
      case Key::Tab: cycle_seed_(); break;
      default: break;
    }
  }

  void tick(uint32_t now_ms) override {
    last_ms_ = now_ms;
    if (paused_) return;
    if (next_ms_ == 0) next_ms_ = now_ms + step_ms_;
    if (now_ms >= next_ms_) {
      engine_.step();
      next_ms_ = now_ms + step_ms_;
    }
  }

  void render(IDisplay& d) override {
    d.clear(ui::kSurface);
    char title[40];
    std::snprintf(title, sizeof(title), "Life g%u  pop %u",
                  engine_.generation(),
                  static_cast<unsigned>(engine_.population()));
    ui::Chrome::header(d, title, paused_ ? "PAUSE" : "RUN");

    d.fill_rect({kBoardX, kBoardY,
                 LifeEngine::kCols * kCellPx,
                 LifeEngine::kRows * kCellPx}, kJapanRedDark);
    for (int y = 0; y < LifeEngine::kRows; ++y) {
      for (int x = 0; x < LifeEngine::kCols; ++x) {
        if (engine_.alive(x, y)) {
          d.fill_rect({kBoardX + x * kCellPx,
                       kBoardY + y * kCellPx,
                       kCellPx, kCellPx}, kJapanRedBright);
        }
      }
    }
    ui::Chrome::footer(d, "Ent=pause Tab=seed Bksp=clear");
    d.flush();
  }

  // Test hooks
  LifeEngine& engine() { return engine_; }
  bool        paused() const { return paused_; }
  uint32_t    step_ms() const { return step_ms_; }

private:
  void cycle_seed_() {
    seed_idx_ = (seed_idx_ + 1) % 4;  // Random, Glider, Blinker, Pulsar
    LifeEngine::Seed s = LifeEngine::Seed::Random;
    switch (seed_idx_) {
      case 0: s = LifeEngine::Seed::Random;  break;
      case 1: s = LifeEngine::Seed::Glider;  break;
      case 2: s = LifeEngine::Seed::Blinker; break;
      case 3: s = LifeEngine::Seed::Pulsar;  break;
    }
    engine_.seed(s, last_ms_ | 1u);
    paused_ = false;
  }

  LifeEngine engine_;
  bool       paused_   = false;
  uint32_t   step_ms_  = 200;
  uint32_t   last_ms_  = 0;
  uint32_t   next_ms_  = 0;
  size_t     seed_idx_ = 1;
};

}  // namespace yui

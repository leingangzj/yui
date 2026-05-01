#pragma once
// Snake. Arrow keys turn, Enter resets after game over. Tick advances the
// engine every kStepMs.
#include "yui/app/App.hpp"
#include "yui/game/SnakeEngine.hpp"
#include "yui/types.hpp"
#include <cstdio>

namespace yui {

class SnakeApp : public App {
public:
  static constexpr uint32_t kStepMs = 140;
  static constexpr int      kCellPx = 8;
  static constexpr int      kBoardX = 0;
  static constexpr int      kBoardY = 16;

  const char* name() const override { return "Snake"; }

  void on_enter(Hal& hal) override {
    last_ms_ = 0;
    next_ms_ = 0;
    engine_.reset(static_cast<uint32_t>(hal.clock.millis()) | 1u);
  }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (engine_.state() == SnakeEngine::State::GameOver && k.key == Key::Enter) {
      engine_.reset(last_ms_ | 1u);
      return;
    }
    switch (k.key) {
      case Key::Up:    engine_.turn(SnakeEngine::Dir::Up);    break;
      case Key::Down:  engine_.turn(SnakeEngine::Dir::Down);  break;
      case Key::Left:  engine_.turn(SnakeEngine::Dir::Left);  break;
      case Key::Right: engine_.turn(SnakeEngine::Dir::Right); break;
      default: break;
    }
  }

  void tick(uint32_t now_ms) override {
    last_ms_ = now_ms;
    if (next_ms_ == 0) next_ms_ = now_ms + kStepMs;
    while (now_ms >= next_ms_) {
      engine_.step();
      next_ms_ += kStepMs;
      if (engine_.state() != SnakeEngine::State::Running) break;
    }
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    char title[40];
    std::snprintf(title, sizeof(title), "Snake  score %d", engine_.score());
    d.draw_text(8, 4, title, kWhite, kJapanRed);

    // Board
    d.fill_rect({kBoardX, kBoardY,
                 SnakeEngine::kCols * kCellPx,
                 SnakeEngine::kRows * kCellPx}, kJapanRedDark);

    // Food
    const auto food = engine_.food();
    d.fill_rect({kBoardX + food.x * kCellPx + 1,
                 kBoardY + food.y * kCellPx + 1,
                 kCellPx - 2, kCellPx - 2}, kJapanRedBright);

    // Body
    for (size_t i = 0; i < engine_.length(); ++i) {
      const auto c = engine_.at(i);
      d.fill_rect({kBoardX + c.x * kCellPx,
                   kBoardY + c.y * kCellPx,
                   kCellPx, kCellPx}, kWhite);
    }

    if (engine_.state() == SnakeEngine::State::GameOver) {
      d.draw_text(d.width() / 2 - 40, d.height() - 14,
                  "GAME OVER  Enter=retry", kWhite, kJapanRed);
    }
    d.flush();
  }

  // Test hooks
  SnakeEngine& engine() { return engine_; }
  const SnakeEngine& engine() const { return engine_; }

private:
  SnakeEngine engine_;
  uint32_t    last_ms_ = 0;
  uint32_t    next_ms_ = 0;
};

}  // namespace yui

#pragma once
// RPN calculator — small, finishable, satisfying with a real keyboard.
//
// Design: a 4-deep stack (T Z Y X). Digit keys append to a build buffer;
// Space pushes the buffer onto the stack; +,-,*,/ pop two and push result;
// Backspace deletes a digit; Enter duplicates X.
//
// All pure logic: no HAL inside CalculatorEngine, render() just paints state.

#include "yui/app/App.hpp"
#include "yui/types.hpp"
#include <array>
#include <cstdio>
#include <cstring>

namespace yui {

class CalculatorEngine {
public:
  static constexpr size_t kStackDepth = 4;
  static constexpr size_t kBufSize    = 24;

  void reset() {
    stack_.fill(0.0);
    sp_ = 0;
    buf_[0] = '\0';
    err_    = false;
  }

  bool error() const { return err_; }
  size_t depth() const { return sp_; }
  double peek(size_t i = 0) const { return (i < sp_) ? stack_[sp_ - 1 - i] : 0.0; }
  const char* buffer() const { return buf_; }

  // Append a single character to the build buffer. Accepts 0-9, '.', '-'.
  void input_char(char c) {
    if (err_) return;
    const size_t n = std::strlen(buf_);
    if (n + 1 >= kBufSize) return;
    if (!is_input_char_(c)) return;
    if (c == '.' && std::strchr(buf_, '.')) return;       // one decimal max
    if (c == '-' && n != 0)                  return;      // sign only at start
    buf_[n] = c;
    buf_[n + 1] = '\0';
  }

  void backspace() {
    if (err_) { err_ = false; return; }
    const size_t n = std::strlen(buf_);
    if (n == 0) {
      // Drop top of stack as a soft undo.
      if (sp_ > 0) --sp_;
      return;
    }
    buf_[n - 1] = '\0';
  }

  // Push the build buffer onto the stack (if non-empty).
  bool flush_buffer() {
    if (err_ || buf_[0] == '\0') return false;
    if (sp_ >= kStackDepth) {
      // Drop oldest to keep the top.
      for (size_t i = 1; i < kStackDepth; ++i) stack_[i - 1] = stack_[i];
      --sp_;
    }
    char* end = nullptr;
    const double v = std::strtod(buf_, &end);
    if (end == buf_) { err_ = true; return false; }
    stack_[sp_++] = v;
    buf_[0] = '\0';
    return true;
  }

  // Duplicate top of stack.
  void dup() {
    if (err_) return;
    flush_buffer();
    if (sp_ == 0) return;
    if (sp_ < kStackDepth) {
      stack_[sp_] = stack_[sp_ - 1];
      ++sp_;
    }
  }

  void op(char which) {
    if (err_) return;
    flush_buffer();
    if (sp_ < 2) { err_ = true; return; }
    const double b = stack_[--sp_];
    const double a = stack_[--sp_];
    double r = 0.0;
    switch (which) {
      case '+': r = a + b; break;
      case '-': r = a - b; break;
      case '*': r = a * b; break;
      case '/':
        if (b == 0.0) { err_ = true; return; }
        r = a / b;
        break;
      default: err_ = true; return;
    }
    stack_[sp_++] = r;
  }

private:
  static bool is_input_char_(char c) {
    return (c >= '0' && c <= '9') || c == '.' || c == '-';
  }

  std::array<double, kStackDepth> stack_{};
  size_t sp_ = 0;
  char   buf_[kBufSize] = {0};
  bool   err_ = false;
};

class CalculatorApp : public App {
public:
  CalculatorApp() = default;
  const char* name() const override { return "Calculator"; }

  void on_enter(Hal& /*hal*/) override { engine_.reset(); }

  void on_key(KeyEvent k) override {
    if (!k.down) return;
    if (k.ch >= '0' && k.ch <= '9') { engine_.input_char(k.ch); return; }
    switch (k.ch) {
      case '.': case '-': engine_.input_char(k.ch); return;
      case '+': engine_.op('+'); return;
      case '*': engine_.op('*'); return;
      case '/': engine_.op('/'); return;
      default: break;
    }
    if (k.key == Key::Backspace) engine_.backspace();
    else if (k.key == Key::Enter) {
      if (!engine_.flush_buffer()) engine_.dup();
    }
    else if (k.key == Key::Space) engine_.flush_buffer();
  }

  void render(IDisplay& d) override {
    d.clear(kWhite);
    d.fill_rect({0, 0, d.width(), 16}, kJapanRed);
    d.draw_text(8, 4, "Calculator", kWhite, kJapanRed);

    if (engine_.error()) {
      d.draw_text(8, 40, "ERR", kJapanRed, kWhite);
      d.draw_text(8, 60, "Backspace clears", kJapanRedDark, kWhite);
      d.flush();
      return;
    }

    // Stack — show top 4, with the topmost (X) at the bottom row.
    const size_t depth = engine_.depth();
    char line[32];
    for (size_t i = 0; i < CalculatorEngine::kStackDepth; ++i) {
      const int y = 24 + static_cast<int>(i) * 14;
      const char* tag = (i == 3) ? "X" : (i == 2) ? "Y" : (i == 1) ? "Z" : "T";
      const size_t depth_i = CalculatorEngine::kStackDepth - 1 - i;
      if (depth_i < depth) {
        std::snprintf(line, sizeof(line), "%s: %g", tag, engine_.peek(depth_i));
      } else {
        std::snprintf(line, sizeof(line), "%s:", tag);
      }
      d.draw_text(8, y, line, kBlack, kWhite);
    }

    // Input buffer
    char buf_line[40];
    std::snprintf(buf_line, sizeof(buf_line), "> %s", engine_.buffer());
    d.draw_text(8, 110, buf_line, kJapanRed, kWhite);
    d.flush();
  }

  CalculatorEngine& engine() { return engine_; }

private:
  CalculatorEngine engine_;
};

}  // namespace yui

#pragma once
// RPN calculator — small, finishable, satisfying with a real keyboard.
//
// Design: a 4-deep stack (T Z Y X). Digit keys append to a build buffer;
// Space pushes the buffer onto the stack; +,-,*,/ pop two and push result;
// Backspace deletes a digit; Enter duplicates X.
//
// Unary ops: 'n' = negate, 'q' = sqrt, 's' = swap top two, 'c' = clear all.
// Trig: 'i' = sin, 'o' = cos, 't' = tan. Default angle mode is radians;
// 'd' toggles to degrees (the header shows "DEG" when active).
// Constants: 'g' = push pi, 'e' = push e.
// Memory register (one slot): 'p' = M-store (top of X), 'r' = M-recall
// (push memory onto stack), 'k' = M-clear.
// To enter a negative literal: type the positive number then 'n'. This
// avoids the parse ambiguity between "negative sign" and "subtract".
//
// All pure logic: no HAL inside CalculatorEngine, render() just paints state.

#include "yui/app/App.hpp"
#include "yui/types.hpp"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace yui {

class CalculatorEngine {
public:
  static constexpr size_t kStackDepth = 4;
  static constexpr size_t kBufSize    = 24;

  enum class AngleMode { Radians, Degrees };

  void reset() {
    stack_.fill(0.0);
    sp_ = 0;
    buf_[0] = '\0';
    err_    = false;
    mem_    = 0.0;
    mem_set_ = false;
    // Angle mode is intentionally preserved across reset — the user's
    // last preference is sticky.
  }

  bool error() const { return err_; }
  size_t depth() const { return sp_; }
  double peek(size_t i = 0) const { return (i < sp_) ? stack_[sp_ - 1 - i] : 0.0; }
  const char* buffer() const { return buf_; }

  // Append a single character to the build buffer. Accepts 0-9 and '.'.
  // Unary negate ('n') is the way to enter negative literals.
  void input_char(char c) {
    if (err_) return;
    const size_t n = std::strlen(buf_);
    if (n + 1 >= kBufSize) return;
    if (!is_input_char_(c)) return;
    if (c == '.' && std::strchr(buf_, '.')) return;       // one decimal max
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

  // Negate top of stack (or buffer, if a number is being typed).
  void neg() {
    if (err_) return;
    const size_t n = std::strlen(buf_);
    if (n > 0) {
      if (buf_[0] == '-') std::memmove(buf_, buf_ + 1, n);     // strip leading '-'
      else if (n + 1 < kBufSize) {
        std::memmove(buf_ + 1, buf_, n + 1);
        buf_[0] = '-';
      }
      return;
    }
    if (sp_ == 0) return;
    stack_[sp_ - 1] = -stack_[sp_ - 1];
  }

  // Square root of top of stack (after flushing any pending input).
  void sqrt_top() {
    if (err_) return;
    flush_buffer();
    if (sp_ == 0) { err_ = true; return; }
    const double v = stack_[sp_ - 1];
    if (v < 0.0) { err_ = true; return; }
    stack_[sp_ - 1] = std::sqrt(v);
  }

  AngleMode angle_mode() const { return angle_; }
  void toggle_angle_mode() {
    angle_ = (angle_ == AngleMode::Radians) ? AngleMode::Degrees
                                            : AngleMode::Radians;
  }
  // Trig honors the current angle mode. Sets err_ if stack is empty.
  void apply_trig_(double (*f)(double)) {
    if (err_) return;
    flush_buffer();
    if (sp_ == 0) { err_ = true; return; }
    double x = stack_[sp_ - 1];
    if (angle_ == AngleMode::Degrees) x *= 0.017453292519943295;  // π/180
    stack_[sp_ - 1] = f(x);
  }
  void sin_top() { apply_trig_(static_cast<double(*)(double)>(std::sin)); }
  void cos_top() { apply_trig_(static_cast<double(*)(double)>(std::cos)); }
  void tan_top() { apply_trig_(static_cast<double(*)(double)>(std::tan)); }

  // Push a constant onto the stack (after flushing any pending input).
  void push_const_(double v) {
    if (err_) return;
    flush_buffer();
    if (sp_ >= kStackDepth) {
      for (size_t i = 1; i < kStackDepth; ++i) stack_[i - 1] = stack_[i];
      --sp_;
    }
    stack_[sp_++] = v;
  }
  void push_pi() { push_const_(3.141592653589793); }
  void push_e()  { push_const_(2.718281828459045); }

  // Memory register (single slot).
  void m_store() {
    if (err_) return;
    flush_buffer();
    if (sp_ == 0) { err_ = true; return; }
    mem_     = stack_[sp_ - 1];
    mem_set_ = true;
  }
  void m_recall() {
    if (err_) return;
    flush_buffer();
    if (sp_ >= kStackDepth) {
      for (size_t i = 1; i < kStackDepth; ++i) stack_[i - 1] = stack_[i];
      --sp_;
    }
    stack_[sp_++] = mem_;
  }
  void m_clear() { mem_ = 0.0; mem_set_ = false; }
  bool   m_has() const { return mem_set_; }
  double m_value() const { return mem_; }

  // Swap X and Y.
  void swap() {
    if (err_) return;
    flush_buffer();
    if (sp_ < 2) { err_ = true; return; }
    const double tmp = stack_[sp_ - 1];
    stack_[sp_ - 1] = stack_[sp_ - 2];
    stack_[sp_ - 2] = tmp;
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
        if (b == 0.0) {
          err_ = true;
          // Restore stack so the user keeps their inputs after clearing the err.
          stack_[sp_++] = a; stack_[sp_++] = b;
          return;
        }
        r = a / b;
        break;
      default: err_ = true; return;
    }
    stack_[sp_++] = r;
  }

private:
  static bool is_input_char_(char c) {
    return (c >= '0' && c <= '9') || c == '.';
  }

  std::array<double, kStackDepth> stack_{};
  size_t sp_ = 0;
  char   buf_[kBufSize] = {0};
  bool   err_ = false;
  double mem_     = 0.0;
  bool   mem_set_ = false;
  AngleMode angle_ = AngleMode::Radians;
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
      case '.': engine_.input_char(k.ch); return;
      case '+': engine_.op('+'); return;
      case '-': engine_.op('-'); return;
      case '*': engine_.op('*'); return;
      case '/': engine_.op('/'); return;
      case 'n': engine_.neg();      return;
      case 'q': engine_.sqrt_top(); return;
      case 's': engine_.swap();     return;
      case 'c': engine_.reset();    return;
      case 'i': engine_.sin_top();  return;
      case 'o': engine_.cos_top();  return;
      case 't': engine_.tan_top();  return;
      case 'd': engine_.toggle_angle_mode(); return;
      case 'g': engine_.push_pi();  return;
      case 'e': engine_.push_e();   return;
      case 'p': engine_.m_store();  return;
      case 'r': engine_.m_recall(); return;
      case 'k': engine_.m_clear();  return;
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

    // Input buffer + memory indicator
    char buf_line[40];
    std::snprintf(buf_line, sizeof(buf_line), "> %s", engine_.buffer());
    d.draw_text(8, 110, buf_line, kJapanRed, kWhite);
    if (engine_.m_has()) d.draw_text(d.width() - 18, 4, "M", kWhite, kJapanRed);
    if (engine_.angle_mode() == CalculatorEngine::AngleMode::Degrees)
      d.draw_text(d.width() - 50, 4, "DEG", kWhite, kJapanRed);
    d.flush();
  }

  CalculatorEngine& engine() { return engine_; }

private:
  CalculatorEngine engine_;
};

}  // namespace yui

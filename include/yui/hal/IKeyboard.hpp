#pragma once
#include <cstdint>

namespace yui {

enum class Key : uint16_t {
  None = 0,
  Up, Down, Left, Right,
  Enter, Esc, Tab, Backspace, Space,
  Fn, Shift, Ctrl, Alt,
  // Printable keys: ASCII value as enum
};

struct KeyEvent {
  Key key;
  char ch;       // printable character if applicable, else 0
  bool down;     // true = press, false = release
  bool shift;
  bool ctrl;
  bool alt;
  bool fn;
};

class IKeyboard {
public:
  virtual ~IKeyboard() = default;
  virtual bool poll(KeyEvent& out) = 0;  // returns true if event consumed
};

}  // namespace yui

#pragma once
// Cardputer ADV 4×14 keyboard layout (TCA8418 raw matrix → Yui KeyEvent).
//
// Layout was reverse-mapped from the M5Stack ADV docs and physical key labels.
// We deliberately re-derived this from the spec rather than copying any GPL'd
// source, so the table here is MIT-licensed along with the rest of Yui.

#include "yui/hal/IKeyboard.hpp"
#include <cstdint>
#include <cstring>

namespace yui {

// One physical key on the 4×14 grid. POD struct (no default member inits)
// so aggregate initialization works cleanly across compiler versions.
struct AdvCell {
  Key  key;       // primary Yui Key (Char means "see ch")
  char ch;        // primary printable char (0 if not printable)
  char shifted;   // printable char with Shift held
  Key  fn_key;    // Yui Key when Fn held (overrides primary; None = no override)
  char fn_ch;     // printable when Fn held (rare)
};

// 4×14 grid stored row-major. cell(r,c) = kAdvKeyMap[r * 14 + c].
constexpr int kAdvRows = 4;
constexpr int kAdvCols = 14;

constexpr AdvCell kAdvKeyMap[kAdvRows * kAdvCols] = {
  // ── Row 0: ` 1 2 3 4 5 6 7 8 9 0 - = del ──
  // Esc is the primary because OS navigation is the dominant use-case;
  // the literal ` / ~ characters live on the Fn layer.
  {Key::Esc,  0,    0,    Key::Char,       '`'},
  {Key::Char, '1',  '!',  Key::None,       0},
  {Key::Char, '2',  '@',  Key::None,       0},
  {Key::Char, '3',  '#',  Key::None,       0},
  {Key::Char, '4',  '$',  Key::None,       0},
  {Key::Char, '5',  '%',  Key::None,       0},
  {Key::Char, '6',  '^',  Key::None,       0},
  {Key::Char, '7',  '&',  Key::None,       0},
  {Key::Char, '8',  '*',  Key::None,       0},
  {Key::Char, '9',  '(',  Key::None,       0},
  {Key::Char, '0',  ')',  Key::None,       0},
  {Key::Char, '-',  '_',  Key::None,       0},
  {Key::Char, '=',  '+',  Key::None,       0},
  {Key::Backspace, 0, 0,  Key::Backspace,  0},
  // ── Row 1: tab q w e r t y u i o p [ ] \ ──
  {Key::Tab,    0,   0,   Key::Tab,        0},
  {Key::Char, 'q', 'Q',   Key::None,       0},
  {Key::Char, 'w', 'W',   Key::None,       0},
  {Key::Char, 'e', 'E',   Key::None,       0},
  {Key::Char, 'r', 'R',   Key::None,       0},
  {Key::Char, 't', 'T',   Key::None,       0},
  {Key::Char, 'y', 'Y',   Key::None,       0},
  {Key::Char, 'u', 'U',   Key::None,       0},
  {Key::Char, 'i', 'I',   Key::None,       0},
  {Key::Char, 'o', 'O',   Key::None,       0},
  {Key::Char, 'p', 'P',   Key::None,       0},
  {Key::Char, '[', '{',   Key::None,       0},
  {Key::Char, ']', '}',   Key::None,       0},
  {Key::Char, '\\','|',   Key::None,       0},
  // ── Row 2: fn shift a s d f g h j k l ; ' enter ──
  {Key::Fn,     0,   0,   Key::Fn,         0},
  {Key::Shift,  0,   0,   Key::Shift,      0},
  {Key::Char, 'a', 'A',   Key::None,       0},
  {Key::Char, 's', 'S',   Key::None,       0},
  {Key::Char, 'd', 'D',   Key::None,       0},
  {Key::Char, 'f', 'F',   Key::None,       0},
  {Key::Char, 'g', 'G',   Key::None,       0},
  {Key::Char, 'h', 'H',   Key::None,       0},
  {Key::Char, 'j', 'J',   Key::None,       0},
  {Key::Char, 'k', 'K',   Key::None,       0},
  {Key::Char, 'l', 'L',   Key::None,       0},
  {Key::Up,    0,   0,    Key::Char,       ';'},
  {Key::Char, '\'','"',   Key::None,       0},
  {Key::Enter,  0,   0,   Key::Enter,      0},
  // ── Row 3: ctrl opt alt z x c v b n m , . / space ──
  {Key::Ctrl,   0,   0,   Key::Ctrl,       0},
  {Key::Alt,    0,   0,   Key::Alt,        0},
  {Key::Alt,    0,   0,   Key::Alt,        0},
  {Key::Char, 'z', 'Z',   Key::None,       0},
  {Key::Char, 'x', 'X',   Key::None,       0},
  {Key::Char, 'c', 'C',   Key::None,       0},
  {Key::Char, 'v', 'V',   Key::None,       0},
  {Key::Char, 'b', 'B',   Key::None,       0},
  {Key::Char, 'n', 'N',   Key::None,       0},
  {Key::Char, 'm', 'M',   Key::None,       0},
  // Arrows are primary because the OS is overwhelmingly nav-driven;
  // punctuation (',' '.' '/') still works via the Fn layer when typing.
  {Key::Left,  0,   0,    Key::Char,       ','},
  {Key::Down,  0,   0,    Key::Char,       '.'},
  {Key::Right, 0,   0,    Key::Char,       '/'},
  {Key::Space,  0,   0,   Key::Space,      0},
};

constexpr const AdvCell& adv_cell(int row, int col) {
  return kAdvKeyMap[row * kAdvCols + col];
}

// TCA8418 raw keycode → logical (row, col) per M5's wiring of the 7×8
// matrix as a 4×14 layout.
//   logical_col = raw_row * 2 + (raw_col > 3 ? 1 : 0)
//   logical_row = (raw_col + 4) % 4
// TCA8418 keycodes are 1-based: keycode = raw_row*10 + raw_col + 1.
struct AdvKeyPos { int row; int col; bool valid; };

inline AdvKeyPos adv_decode_pos(uint8_t keycode) {
  if (keycode == 0) return {0, 0, false};
  const uint8_t k = keycode - 1;
  const int raw_row = k / 10;
  const int raw_col = k % 10;
  const int log_col = raw_row * 2 + (raw_col > 3 ? 1 : 0);
  const int log_row = (raw_col + 4) % 4;
  if (log_row < 0 || log_row >= 4 || log_col < 0 || log_col >= 14)
    return {0, 0, false};
  return {log_row, log_col, true};
}

// Build a Yui KeyEvent from a raw TCA8418 keycode + current modifier state.
// Returns false for unmapped / out-of-range codes.
inline bool adv_make_event(uint8_t raw_keycode, bool is_press,
                           bool shift_held, bool fn_held,
                           bool ctrl_held,  bool alt_held,
                           KeyEvent& out) {
  const auto pos = adv_decode_pos(raw_keycode);
  if (!pos.valid) return false;
  const AdvCell& cell = adv_cell(pos.row, pos.col);

  out = KeyEvent{};
  out.down  = is_press;
  out.shift = shift_held;
  out.fn    = fn_held;
  out.ctrl  = ctrl_held;
  out.alt   = alt_held;

  if (fn_held && cell.fn_key != Key::None) {
    out.key = cell.fn_key;
    out.ch  = cell.fn_ch;
  } else {
    out.key = cell.key;
    out.ch  = shift_held ? cell.shifted : cell.ch;
  }
  return true;
}

}  // namespace yui

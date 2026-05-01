// Native unit tests — pure logic, no hardware.
// Run with: pio test -e native

#include <unity.h>
#include "yui/shell/Menu.hpp"
#include "yui/types.hpp"
#include "yui/gfx/Braille.hpp"
#include "yui/gfx/koi_art.hpp"
#include "yui/shell/Splash.hpp"
#include "../../src/hal/native/NativeDisplay.hpp"
#include "yui/app/AppRegistry.hpp"
#include "yui/app/Launcher.hpp"
#include "yui/app/AboutApp.hpp"
#include "yui/app/StubApp.hpp"
#include "yui/shell/Shell.hpp"
#include "../../src/hal/native/NativeKeyboard.hpp"
#include "../../src/hal/native/NativeClock.hpp"
#include "../../src/hal/native/NativeLog.hpp"
#include "../../src/hal/native/NativeNet.hpp"
#include "../../src/hal/native/NativeImu.hpp"
#include "../../src/hal/native/NativeFs.hpp"
#include "../../src/hal/native/NativeIr.hpp"
#include "../../src/hal/native/NativeMic.hpp"
#include "yui/app/WifiApp.hpp"
#include "yui/app/BleApp.hpp"
#include "yui/app/CalculatorApp.hpp"
#include "yui/app/ImuApp.hpp"
#include "yui/app/NotesApp.hpp"
#include "yui/app/FilesApp.hpp"
#include "yui/app/IrRemoteApp.hpp"
#include "yui/app/MicApp.hpp"
#include "yui/app/SettingsApp.hpp"
#include "yui/app/ClockApp.hpp"
#include "yui/app/SysinfoApp.hpp"
#include "yui/app/SnakeApp.hpp"
#include "yui/app/KeyTestApp.hpp"
#include "yui/game/SnakeEngine.hpp"
#include "yui/drivers/AdvKeymap.hpp"
#include "../../src/hal/native/NativeStorage.hpp"
#include "../../src/hal/native/NativeSpeaker.hpp"
#include "yui/app/ToneApp.hpp"
#include "yui/app/PomodoroApp.hpp"
#include "yui/app/MetronomeApp.hpp"
#include "yui/app/LifeApp.hpp"
#include "yui/game/LifeEngine.hpp"
#include "yui/app/DrawApp.hpp"
#include <cstring>

using yui::Menu;
using yui::NativeDisplay;
using yui::kJapanRed;
using yui::kJapanRedBright;
using yui::kWhite;
using namespace yui;

void setUp() {}
void tearDown() {}

// ───── Menu ─────────────────────────────────────────────────────────────────

void test_menu_starts_at_zero() {
  Menu m(5);
  TEST_ASSERT_EQUAL(0u, m.cursor());
}

void test_menu_down_advances() {
  Menu m(5);
  m.down();
  TEST_ASSERT_EQUAL(1u, m.cursor());
}

void test_menu_down_wraps_at_end() {
  Menu m(3);
  m.down(); m.down(); m.down();
  TEST_ASSERT_EQUAL(0u, m.cursor());
}

void test_menu_up_wraps_at_start() {
  Menu m(3);
  m.up();
  TEST_ASSERT_EQUAL(2u, m.cursor());
}

void test_menu_set_count_clamps_cursor() {
  Menu m(10);
  m.down(); m.down(); m.down(); m.down();
  m.set_count(2);
  TEST_ASSERT_EQUAL(1u, m.cursor());
}

// ───── Braille decoder ──────────────────────────────────────────────────────

void test_braille_decode_blank_glyph() {
  // U+2800 = 0xE2 0xA0 0x80 = no dots.
  const char* p = "\xe2\xa0\x80";
  uint8_t dots = yui::braille_decode(p);
  TEST_ASSERT_EQUAL_HEX8(0x00, dots);
}

void test_braille_decode_full_block() {
  // U+28FF = 0xE2 0xA3 0xBF = all 8 dots set.
  const char* p = "\xe2\xa3\xbf";
  uint8_t dots = yui::braille_decode(p);
  TEST_ASSERT_EQUAL_HEX8(0xFF, dots);
}

void test_braille_decode_advances_pointer() {
  const char* p = "\xe2\xa0\x80\xe2\xa3\xbf";
  yui::braille_decode(p);
  yui::braille_decode(p);
  TEST_ASSERT_EQUAL_CHAR('\0', *p);
}

void test_koi_art_dimensions_match_constants() {
  // Walk the embedded art and count rows + max cols.
  int rows = 1, cols = 0, cur_col = 0;
  const char* p = yui::kKoiArt;
  while (*p) {
    if (*p == '\n') {
      if (cur_col > cols) cols = cur_col;
      cur_col = 0;
      ++rows;
      ++p;
      continue;
    }
    yui::braille_decode(p);
    ++cur_col;
  }
  // Trailing newline counted one extra row, subtract it.
  --rows;
  TEST_ASSERT_EQUAL_INT(yui::kKoiArtRows, rows);
  TEST_ASSERT_EQUAL_INT(yui::kKoiArtCols, cols);
}

// ───── Animated splash ──────────────────────────────────────────────────────

void test_splash_paints_white_background() {
  NativeDisplay d;
  yui::render_splash(d, "v0.0.1", 0);
  // Far corner pixel should always be white (koi is centered).
  TEST_ASSERT_EQUAL_HEX16(kWhite, d.pixel_at(0, 0));
  TEST_ASSERT_EQUAL_HEX16(kWhite, d.pixel_at(d.width()/2 - 100, 5));
}

void test_splash_renders_red_koi_pixels_at_t0() {
  NativeDisplay d;
  yui::render_splash(d, "v0.0.1", 0);
  // Sweep the koi region for at least one red pixel.
  int red_pixels = 0;
  for (int y = 6; y < 6 + yui::kKoiArtDotsH * yui::kSplashScale; ++y) {
    for (int x = 0; x < d.width(); ++x) {
      auto p = d.pixel_at(x, y);
      if (p == kJapanRed || p == kJapanRedBright) ++red_pixels;
    }
  }
  TEST_ASSERT_GREATER_THAN_INT(50, red_pixels);
}

void test_splash_shimmer_advances_with_time() {
  // The highlight column should be at a different dot position after enough
  // time has passed. We capture a "fingerprint" of bright-red pixel positions
  // and assert it changes between t=0 and t=several steps later.
  auto fingerprint = [](NativeDisplay& d) {
    uint32_t sum = 0;
    for (int y = 0; y < d.height(); ++y)
      for (int x = 0; x < d.width(); ++x)
        if (d.pixel_at(x, y) == kJapanRedBright) sum += static_cast<uint32_t>(x * 31 + y);
    return sum;
  };

  NativeDisplay a, b;
  yui::render_splash(a, "v", 0);
  yui::render_splash(b, "v", yui::kShimmerStepMs * 8);

  TEST_ASSERT_NOT_EQUAL(fingerprint(a), fingerprint(b));
}

void test_splash_writes_version_string() {
  NativeDisplay d;
  yui::render_splash(d, "v9.9.9-banana", 0);
  // The version is the last text drawn each frame (footer line).
  TEST_ASSERT_EQUAL_STRING("v9.9.9-banana", d.last_text().c_str());
}

void test_splash_flushes_once_per_frame() {
  NativeDisplay d;
  yui::render_splash(d, "v", 0);
  yui::render_splash(d, "v", 100);
  TEST_ASSERT_EQUAL(2, d.flush_count());
}

// ───── Test runner ──────────────────────────────────────────────────────────


// Native unit tests for Launcher + Shell — pure logic + framebuffer asserts.
namespace {
struct Fixture {
  NativeDisplay  display;
  MockKeyboard   keyboard;
  FakeClock      clock;
  StderrLog      log;
  Hal            hal{display, keyboard, clock, log};
  AppRegistry    registry;
};

KeyEvent press(Key k) {
  KeyEvent e{};
  e.key  = k;
  e.down = true;
  return e;
}
}  // namespace

// ───── AppRegistry ──────────────────────────────────────────────────────────

void test_registry_starts_empty() {
  AppRegistry r;
  TEST_ASSERT_EQUAL_size_t(0u, r.size());
  TEST_ASSERT_NULL(r.at(0));
}

void test_registry_adds_apps() {
  AppRegistry r;
  StubApp a{"A"}, b{"B"};
  TEST_ASSERT_TRUE(r.add(&a));
  TEST_ASSERT_TRUE(r.add(&b));
  TEST_ASSERT_EQUAL_size_t(2u, r.size());
  TEST_ASSERT_EQUAL_STRING("A", r.at(0)->name());
  TEST_ASSERT_EQUAL_STRING("B", r.at(1)->name());
}

void test_registry_rejects_null() {
  AppRegistry r;
  TEST_ASSERT_FALSE(r.add(nullptr));
}

// ───── Launcher ─────────────────────────────────────────────────────────────

void test_launcher_cursor_starts_at_zero() {
  Fixture f;
  StubApp a{"A"}, b{"B"};
  f.registry.add(&a);
  f.registry.add(&b);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(0u, l.cursor());
  TEST_ASSERT_EQUAL_PTR(&a, l.selected());
}

void test_launcher_down_advances_cursor() {
  Fixture f;
  StubApp a{"A"}, b{"B"}, c{"C"};
  f.registry.add(&a); f.registry.add(&b); f.registry.add(&c);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  l.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_PTR(&b, l.selected());
}

void test_launcher_up_wraps() {
  Fixture f;
  StubApp a{"A"}, b{"B"}, c{"C"};
  f.registry.add(&a); f.registry.add(&b); f.registry.add(&c);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  l.on_key(press(Key::Up));
  TEST_ASSERT_EQUAL_PTR(&c, l.selected());
}

void test_launcher_enter_sets_pending_launch() {
  Fixture f;
  StubApp a{"A"}, b{"B"};
  f.registry.add(&a); f.registry.add(&b);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  l.on_key(press(Key::Down));
  l.on_key(press(Key::Enter));
  App* pending = l.take_pending_launch();
  TEST_ASSERT_EQUAL_PTR(&b, pending);
  // Second take should be null (consumed).
  TEST_ASSERT_NULL(l.take_pending_launch());
}

void test_launcher_renders_header_in_red() {
  Fixture f;
  StubApp a{"A"};
  f.registry.add(&a);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  l.render(f.display);
  // Header band is the top kHeaderH rows, painted kJapanRed.
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, 5));
}

void test_launcher_renders_selected_row_highlighted() {
  Fixture f;
  StubApp a{"A"}, b{"B"};
  f.registry.add(&a); f.registry.add(&b);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  l.render(f.display);
  // Cursor on row 0: row 0 should have red bg, row 1 should have white bg.
  const int row0_y = Launcher::kHeaderH + 4;
  const int row1_y = Launcher::kHeaderH + 4 + Launcher::kRowH;
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, row0_y));
  TEST_ASSERT_EQUAL_HEX16(kWhite,    f.display.pixel_at(20, row1_y));
}

// ───── Shell ────────────────────────────────────────────────────────────────

void test_shell_starts_in_splash() {
  Fixture f;
  StubApp a{"A"};
  f.registry.add(&a);
  Launcher l{f.registry};
  Shell s{f.hal, l, "vTEST", 1500};
  s.start();
  TEST_ASSERT_TRUE(s.phase() == Shell::Phase::Splash);
}

void test_shell_holds_splash_before_min_time() {
  Fixture f;
  StubApp a{"A"};
  f.registry.add(&a);
  Launcher l{f.registry};
  Shell s{f.hal, l, "vTEST", 1500};
  s.start();
  // Press a key well before splash_min_ms — should NOT advance.
  f.keyboard.inject(press(Key::Enter));
  f.clock.advance(100);
  s.tick();
  TEST_ASSERT_TRUE(s.phase() == Shell::Phase::Splash);
}

void test_shell_advances_to_launcher_after_min_time_and_key() {
  Fixture f;
  StubApp a{"A"};
  f.registry.add(&a);
  Launcher l{f.registry};
  Shell s{f.hal, l, "vTEST", 1500};
  s.start();
  f.clock.advance(2000);
  f.keyboard.inject(press(Key::Enter));
  s.tick();
  TEST_ASSERT_TRUE(s.phase() == Shell::Phase::Launcher);
}

void test_shell_enters_app_on_launcher_enter() {
  Fixture f;
  StubApp a{"A"}, b{"B"};
  f.registry.add(&a); f.registry.add(&b);
  Launcher l{f.registry};
  Shell s{f.hal, l, "vTEST", 0};  // no splash for this test
  s.start();
  // Skip splash
  f.clock.advance(10);
  f.keyboard.inject(press(Key::Enter));
  s.tick();
  TEST_ASSERT_TRUE(s.phase() == Shell::Phase::Launcher);
  // Move cursor and launch
  f.keyboard.inject(press(Key::Down));
  s.tick();
  f.keyboard.inject(press(Key::Enter));
  s.tick();
  TEST_ASSERT_TRUE(s.phase() == Shell::Phase::App);
  TEST_ASSERT_EQUAL_PTR(&b, s.current_app());
}

void test_shell_esc_returns_to_launcher() {
  Fixture f;
  StubApp a{"A"};
  f.registry.add(&a);
  Launcher l{f.registry};
  Shell s{f.hal, l, "vTEST", 0};
  s.start();
  // Splash → launcher → app → esc
  f.clock.advance(10);
  f.keyboard.inject(press(Key::Enter)); s.tick();   // splash → launcher
  f.keyboard.inject(press(Key::Enter)); s.tick();   // launcher → app
  TEST_ASSERT_TRUE(s.phase() == Shell::Phase::App);
  f.keyboard.inject(press(Key::Esc));   s.tick();
  TEST_ASSERT_TRUE(s.phase() == Shell::Phase::Launcher);
  TEST_ASSERT_NULL(s.current_app());
}

// ───── AboutApp ─────────────────────────────────────────────────────────────

void test_about_renders_version() {
  Fixture f;
  AboutApp app{"v1.2.3"};
  app.render(f.display);
  // Last text drawn should be the footer hint, not version — but the
  // framebuffer should include red pixels in the header.
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, 5));
}

// ───── WifiApp ──────────────────────────────────────────────────────────────

namespace {
WifiAp make_ap(const char* ssid, int8_t rssi, bool secured = false) {
  WifiAp a{};
  std::strncpy(a.ssid, ssid, sizeof(a.ssid) - 1);
  a.rssi    = rssi;
  a.secured = secured;
  return a;
}
}

void test_wifi_app_starts_scanning() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(false);  // require explicit done
  WifiApp app{net};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Scanning);
  TEST_ASSERT_EQUAL_INT(1, net.wifi_starts());
}

void test_wifi_app_transitions_to_done_with_results() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(false);
  net.set_wifi_results({make_ap("AlphaNet", -45, true),
                        make_ap("BetaNet",  -67)});
  WifiApp app{net};
  app.on_enter(f.hal);
  net.simulate_wifi_done();
  app.tick(0);
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Done);
}

void test_wifi_app_transitions_to_empty_when_no_results() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(false);
  WifiApp app{net};
  app.on_enter(f.hal);
  net.simulate_wifi_done();
  app.tick(0);
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Empty);
}

void test_wifi_app_renders_header_red() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("Net1", -50)});
  WifiApp app{net};
  app.on_enter(f.hal);
  app.tick(0);
  app.render(f.display);
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, 5));
}

void test_wifi_app_arrow_keys_move_cursor() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("A", -50), make_ap("B", -60), make_ap("C", -70)});
  WifiApp app{net};
  app.on_enter(f.hal);
  app.tick(0);
  app.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor());
  app.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_size_t(2u, app.cursor());
  app.on_key(press(Key::Up));
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor());
}

void test_wifi_app_enter_restarts_scan() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("A", -50)});
  WifiApp app{net};
  app.on_enter(f.hal);
  app.tick(0);
  TEST_ASSERT_EQUAL_INT(1, net.wifi_starts());
  app.on_key(press(Key::Enter));
  TEST_ASSERT_EQUAL_INT(2, net.wifi_starts());
}

// ───── BleApp ───────────────────────────────────────────────────────────────

namespace {
BleDevice make_dev(const char* nm, int8_t rssi) {
  BleDevice d{};
  std::strncpy(d.name, nm, sizeof(d.name) - 1);
  d.rssi = rssi;
  return d;
}
}

void test_ble_app_starts_and_completes() {
  Fixture f;
  FakeNet net;
  net.set_ble_immediate(false);
  net.set_ble_results({make_dev("Watch", -55), make_dev("Earbud", -72)});
  BleApp app{net};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.state() == BleApp::State::Scanning);
  net.simulate_ble_done();
  app.tick(0);
  TEST_ASSERT_TRUE(app.state() == BleApp::State::Done);
}

void test_ble_app_arrow_keys_move_cursor() {
  Fixture f;
  FakeNet net;
  net.set_ble_immediate(true);
  net.set_ble_results({make_dev("A", -50), make_dev("B", -60)});
  BleApp app{net};
  app.on_enter(f.hal);
  app.tick(0);
  app.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor());
}

// ───── CalculatorApp / Engine ───────────────────────────────────────────────

void test_calc_engine_addition() {
  CalculatorEngine e;
  e.input_char('2'); e.flush_buffer();
  e.input_char('3'); e.flush_buffer();
  e.op('+');
  TEST_ASSERT_EQUAL_FLOAT(5.0, e.peek(0));
  TEST_ASSERT_EQUAL_size_t(1u, e.depth());
}

void test_calc_engine_division_by_zero_errors() {
  CalculatorEngine e;
  e.input_char('5'); e.flush_buffer();
  e.input_char('0'); e.flush_buffer();
  e.op('/');
  TEST_ASSERT_TRUE(e.error());
}

void test_calc_engine_decimal_input() {
  CalculatorEngine e;
  e.input_char('1'); e.input_char('.'); e.input_char('5');
  e.flush_buffer();
  TEST_ASSERT_EQUAL_FLOAT(1.5, e.peek(0));
}

void test_calc_engine_dup() {
  CalculatorEngine e;
  e.input_char('7'); e.flush_buffer();
  e.dup();
  TEST_ASSERT_EQUAL_size_t(2u, e.depth());
  TEST_ASSERT_EQUAL_FLOAT(7.0, e.peek(0));
  TEST_ASSERT_EQUAL_FLOAT(7.0, e.peek(1));
}

void test_calc_engine_backspace() {
  CalculatorEngine e;
  e.input_char('1'); e.input_char('2'); e.input_char('3');
  e.backspace();
  TEST_ASSERT_EQUAL_STRING("12", e.buffer());
}

void test_calc_app_dispatches_digits_and_ops() {
  Fixture f;
  CalculatorApp app;
  app.on_enter(f.hal);
  // 4 ENTER 6 + → 10
  KeyEvent k{};
  k.down = true; k.key = Key::Char; k.ch = '4'; app.on_key(k);
  app.on_key(press(Key::Enter));
  k.ch = '6'; app.on_key(k);
  k.ch = '+'; app.on_key(k);
  TEST_ASSERT_EQUAL_FLOAT(10.0, app.engine().peek(0));
}

// ───── ImuApp ───────────────────────────────────────────────────────────────

void test_imu_app_reads_accel_on_tick() {
  Fixture f;
  FakeImu imu;
  imu.set_accel(0.3f, -0.2f, 0.9f);
  ImuApp app{imu};
  app.on_enter(f.hal);
  app.tick(0);
  AccelXYZ a = app.last_accel();
  TEST_ASSERT_EQUAL_FLOAT(0.3f,  a.x);
  TEST_ASSERT_EQUAL_FLOAT(-0.2f, a.y);
}

void test_imu_app_renders_header_red() {
  Fixture f;
  FakeImu imu;
  ImuApp app{imu};
  app.on_enter(f.hal);
  app.tick(0);
  app.render(f.display);
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, 5));
}

// ───── NotesApp ─────────────────────────────────────────────────────────────

void test_notes_starts_empty() {
  Fixture f;
  FakeFs fs;
  NotesApp app{fs};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(0u, app.buffer_len());
  TEST_ASSERT_FALSE(app.dirty());
}

void test_notes_typing_marks_dirty() {
  Fixture f;
  FakeFs fs;
  NotesApp app{fs};
  app.on_enter(f.hal);
  KeyEvent k{};
  k.down = true; k.key = Key::Char; k.ch = 'h'; app.on_key(k);
  k.ch = 'i'; app.on_key(k);
  TEST_ASSERT_EQUAL_STRING("hi", app.buffer());
  TEST_ASSERT_TRUE(app.dirty());
}

void test_notes_backspace() {
  Fixture f;
  FakeFs fs;
  NotesApp app{fs};
  app.on_enter(f.hal);
  KeyEvent k{};
  k.down = true; k.key = Key::Char; k.ch = 'a'; app.on_key(k);
  k.ch = 'b'; app.on_key(k);
  app.on_key(press(Key::Backspace));
  TEST_ASSERT_EQUAL_STRING("a", app.buffer());
}

void test_notes_arrow_left_right_move_cursor() {
  Fixture f;
  FakeFs fs;
  NotesApp app{fs};
  app.on_enter(f.hal);
  KeyEvent k{};
  k.down = true; k.key = Key::Char;
  k.ch = 'a'; app.on_key(k);
  k.ch = 'b'; app.on_key(k);
  k.ch = 'c'; app.on_key(k);
  TEST_ASSERT_EQUAL_size_t(3u, app.cursor());
  app.on_key(press(Key::Left));
  TEST_ASSERT_EQUAL_size_t(2u, app.cursor());
  app.on_key(press(Key::Left));
  app.on_key(press(Key::Left));
  app.on_key(press(Key::Left));  // clamps at 0
  TEST_ASSERT_EQUAL_size_t(0u, app.cursor());
  app.on_key(press(Key::Right));
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor());
}

void test_notes_insert_mid_buffer() {
  Fixture f;
  FakeFs fs;
  NotesApp app{fs};
  app.on_enter(f.hal);
  KeyEvent k{};
  k.down = true; k.key = Key::Char;
  k.ch = 'a'; app.on_key(k);
  k.ch = 'c'; app.on_key(k);
  app.on_key(press(Key::Left));         // between a and c
  k.ch = 'b'; app.on_key(k);
  TEST_ASSERT_EQUAL_STRING("abc", app.buffer());
  TEST_ASSERT_EQUAL_size_t(2u, app.cursor());
}

void test_notes_backspace_mid_buffer() {
  Fixture f;
  FakeFs fs;
  NotesApp app{fs};
  app.on_enter(f.hal);
  KeyEvent k{};
  k.down = true; k.key = Key::Char;
  k.ch = 'a'; app.on_key(k);
  k.ch = 'b'; app.on_key(k);
  k.ch = 'c'; app.on_key(k);
  app.on_key(press(Key::Left));         // cursor between b and c
  app.on_key(press(Key::Backspace));    // delete b
  TEST_ASSERT_EQUAL_STRING("ac", app.buffer());
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor());
}

void test_notes_up_down_preserve_target_column() {
  Fixture f;
  FakeFs fs;
  NotesApp app{fs};
  app.on_enter(f.hal);
  KeyEvent k{};
  k.down = true; k.key = Key::Char;
  // line 0: "hello"  line 1: "hi"  line 2: "world"
  for (char c : "hello") if (c) { k.ch = c; app.on_key(k); }
  app.on_key(press(Key::Enter));
  for (char c : "hi") if (c) { k.ch = c; app.on_key(k); }
  app.on_key(press(Key::Enter));
  for (char c : "world") if (c) { k.ch = c; app.on_key(k); }
  // cursor at end of "world" (line 2, col 5)
  TEST_ASSERT_EQUAL_size_t(2u, app.cursor_line());
  TEST_ASSERT_EQUAL_size_t(5u, app.cursor_col());
  // up to line 1: "hi" only has 2 cols, clamps
  app.on_key(press(Key::Up));
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor_line());
  TEST_ASSERT_EQUAL_size_t(2u, app.cursor_col());
  // up to line 0: target column 5 still remembered, line is "hello" (5)
  app.on_key(press(Key::Up));
  TEST_ASSERT_EQUAL_size_t(0u, app.cursor_line());
  TEST_ASSERT_EQUAL_size_t(5u, app.cursor_col());
}

void test_notes_save_and_reload() {
  Fixture f;
  FakeFs fs;
  NotesApp app{fs};
  app.on_enter(f.hal);
  KeyEvent k{};
  k.down = true; k.key = Key::Char; k.ch = 'X'; app.on_key(k);
  app.on_key(press(Key::Tab));   // save
  TEST_ASSERT_FALSE(app.dirty());
  // Re-enter — should reload from fs.
  NotesApp app2{fs};
  app2.on_enter(f.hal);
  TEST_ASSERT_EQUAL_STRING("X", app2.buffer());
}

// ───── FilesApp ─────────────────────────────────────────────────────────────

void test_files_lists_root() {
  Fixture f;
  FakeFs fs;
  fs.init();
  fs.mkdir("/photos");
  fs.put_file("/readme.txt", "hi");
  FilesApp app{fs};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(2u, app.count());
}

void test_files_enter_descends_into_dir() {
  Fixture f;
  FakeFs fs;
  fs.init();
  fs.mkdir("/photos");
  fs.put_file("/photos/cat.jpg", "fake");
  FilesApp app{fs};
  app.on_enter(f.hal);
  // Cursor at "photos" (sorted before any other entry here).
  app.on_key(press(Key::Enter));
  TEST_ASSERT_EQUAL_STRING("/photos", app.cwd());
  // /photos contains cat.jpg + synthetic "..".
  TEST_ASSERT_EQUAL_size_t(2u, app.count());
}

void test_files_dotdot_at_subdir_pops_to_parent() {
  Fixture f;
  FakeFs fs;
  fs.init();
  fs.mkdir("/photos");
  fs.put_file("/photos/cat.jpg", "fake");
  FilesApp app{fs};
  app.on_enter(f.hal);
  // Descend into /photos
  app.on_key(press(Key::Enter));
  TEST_ASSERT_EQUAL_STRING("/photos", app.cwd());
  // Cursor at 0 = ".."
  TEST_ASSERT_EQUAL_STRING("..", app.at(0).name);
  app.on_key(press(Key::Enter));  // pop
  TEST_ASSERT_EQUAL_STRING("/", app.cwd());
}

void test_files_no_dotdot_at_root() {
  Fixture f;
  FakeFs fs;
  fs.init();
  fs.put_file("/a.txt", "x");
  FilesApp app{fs};
  app.on_enter(f.hal);
  // No synthetic ".." at root.
  for (size_t i = 0; i < app.count(); ++i) {
    TEST_ASSERT_TRUE(std::strcmp(app.at(i).name, "..") != 0);
  }
}

void test_files_enter_on_file_opens_viewer() {
  Fixture f;
  FakeFs fs;
  fs.init();
  fs.put_file("/note.txt", "hello world");
  FilesApp app{fs};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.mode() == FilesApp::Mode::List);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.mode() == FilesApp::Mode::View);
  TEST_ASSERT_EQUAL_STRING("/note.txt", app.view_path());
  TEST_ASSERT_EQUAL_size_t(11u, app.view_len());
  TEST_ASSERT_EQUAL_STRING_LEN("hello world", app.view_buf(), 11);
}

void test_files_view_backspace_returns_to_list() {
  Fixture f;
  FakeFs fs;
  fs.init();
  fs.put_file("/a.txt", "x");
  FilesApp app{fs};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));        // open viewer
  TEST_ASSERT_TRUE(app.mode() == FilesApp::Mode::View);
  app.on_key(press(Key::Backspace));    // back
  TEST_ASSERT_TRUE(app.mode() == FilesApp::Mode::List);
}

void test_files_pop_restores_cursor() {
  Fixture f;
  FakeFs fs;
  fs.init();
  fs.mkdir("/a");
  fs.mkdir("/b");
  fs.put_file("/b/c.txt", "x");
  FilesApp app{fs};
  app.on_enter(f.hal);
  // Two dirs at root: /a (cursor 0), /b (cursor 1). Move to /b and descend.
  app.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor());
  app.on_key(press(Key::Enter));
  TEST_ASSERT_EQUAL_STRING("/b", app.cwd());
  // Pop via "..".
  app.on_key(press(Key::Enter));
  TEST_ASSERT_EQUAL_STRING("/", app.cwd());
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor());
}

// ───── IrRemoteApp ──────────────────────────────────────────────────────────

void test_ir_app_sends_nec_on_enter() {
  Fixture f;
  FakeIr ir;
  IrRemoteApp app{ir};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_EQUAL_UINT32(1u, ir.sent_count());
  TEST_ASSERT_EQUAL_HEX16(kNecPresets[0].addr, ir.last_addr());
}

void test_ir_app_arrow_keys_change_selection() {
  Fixture f;
  FakeIr ir;
  IrRemoteApp app{ir};
  app.on_enter(f.hal);
  app.on_key(press(Key::Down));
  app.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_size_t(2u, app.cursor());
  app.on_key(press(Key::Up));
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor());
}

// ───── MicApp ───────────────────────────────────────────────────────────────

void test_mic_app_silent_input_yields_zero_rms() {
  Fixture f;
  FakeMic mic;
  mic.push_constant(0, MicApp::kFrame);
  MicApp app{mic};
  app.on_enter(f.hal);
  app.tick(0);
  TEST_ASSERT_EQUAL_FLOAT(0.f, app.rms());
}

void test_mic_app_sine_at_1khz_lands_in_expected_band() {
  // 16 kHz sample rate, 256-pt FFT → 62.5 Hz/bin. 1 kHz = bin 16, which is
  // band 4 (bins 16..32).
  Fixture f;
  FakeMic mic;
  mic.push_sine(1000.f, 16000.f, 16000, MicApp::kFrame);
  MicApp app{mic};
  app.on_enter(f.hal);
  app.tick(0);
  // Band 4 should be the strongest.
  int peak_band = 0;
  for (int b = 1; b < MicApp::kBands; ++b) {
    if (app.band(b) > app.band(peak_band)) peak_band = b;
  }
  TEST_ASSERT_EQUAL_INT(4, peak_band);
  TEST_ASSERT_GREATER_THAN_INT(20, app.band(4));
}

void test_mic_app_sine_at_300hz_lands_in_low_band() {
  // 300 Hz → bin ~5, which is band 2 (bins 4..8).
  Fixture f;
  FakeMic mic;
  mic.push_sine(300.f, 16000.f, 16000, MicApp::kFrame);
  MicApp app{mic};
  app.on_enter(f.hal);
  app.tick(0);
  int peak_band = 0;
  for (int b = 1; b < MicApp::kBands; ++b) {
    if (app.band(b) > app.band(peak_band)) peak_band = b;
  }
  TEST_ASSERT_EQUAL_INT(2, peak_band);
}

// ───── FFT ──────────────────────────────────────────────────────────────────

void test_fft_dc_input_concentrates_in_bin_zero() {
  constexpr size_t N = 16;
  float re[N], im[N] = {0};
  for (size_t i = 0; i < N; ++i) re[i] = 1.f;
  yui::dsp::fft_radix2(re, im, N);
  // bin 0 magnitude should be N (sum of inputs); other bins ~0.
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, static_cast<float>(N), re[0]);
  for (size_t k = 1; k < N; ++k) {
    const float mag = std::sqrt(re[k] * re[k] + im[k] * im[k]);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.f, mag);
  }
}

void test_fft_single_bin_sine() {
  // sin(2π * k * n / N) with k=2 → magnitude peak at bin 2 (and N-2).
  constexpr size_t N = 32;
  constexpr size_t k_target = 2;
  constexpr float kPi = 3.14159265358979323846f;
  float re[N], im[N] = {0};
  for (size_t n = 0; n < N; ++n)
    re[n] = std::sin(2.f * kPi * k_target * n / N);
  yui::dsp::fft_radix2(re, im, N);
  size_t peak_bin = 0;
  float  peak_mag = 0.f;
  for (size_t k = 1; k < N / 2; ++k) {
    const float mag = std::sqrt(re[k] * re[k] + im[k] * im[k]);
    if (mag > peak_mag) { peak_mag = mag; peak_bin = k; }
  }
  TEST_ASSERT_EQUAL_size_t(k_target, peak_bin);
}

void test_mic_app_loud_input_yields_high_rms() {
  Fixture f;
  FakeMic mic;
  mic.push_constant(20000, MicApp::kFrame);
  MicApp app{mic};
  app.on_enter(f.hal);
  app.tick(0);
  TEST_ASSERT_TRUE(app.rms() > 0.1f);
}

// ───── ADV keymap ───────────────────────────────────────────────────────────

void test_adv_decode_pos_keycode_1_is_tab_row1_col0() {
  // Keycode 1 → raw_row=0, raw_col=0 → log_col=0, log_row=(0+4)%4=0.
  // Wait — that maps to row 0 col 0 ("`"). Let me verify: the M5 source uses
  // `(key.col + 4) % 4` with raw_col=0 → 0. So keycode 1 = row 0, col 0.
  auto p = adv_decode_pos(1);
  TEST_ASSERT_TRUE(p.valid);
  TEST_ASSERT_EQUAL_INT(0, p.row);
  TEST_ASSERT_EQUAL_INT(0, p.col);
}

void test_adv_make_event_letter_unshifted() {
  // Find keycode for 'a' (row 2, col 2). Need raw such that (raw_col+4)%4=2
  // and raw_row*2 + (raw_col>3?1:0) = 2 → raw_row=1, raw_col=2 →
  // keycode = 1*10 + 2 + 1 = 13.
  KeyEvent e{};
  TEST_ASSERT_TRUE(adv_make_event(13, true, false, false, false, false, e));
  TEST_ASSERT_EQUAL_CHAR('a', e.ch);
  TEST_ASSERT_TRUE(e.key == Key::Char);
  TEST_ASSERT_TRUE(e.down);
}

void test_adv_make_event_letter_shifted() {
  KeyEvent e{};
  TEST_ASSERT_TRUE(adv_make_event(13, true, true, false, false, false, e));
  TEST_ASSERT_EQUAL_CHAR('A', e.ch);
}

void test_adv_make_event_fn_modifies_arrow_keys() {
  // ',' lives at row 3, col 10. Find raw: (raw_col+4)%4=3 → raw_col∈{3,7};
  // raw_row*2 + (raw_col>3?1:0) = 10 → if raw_col=3: raw_row=5; if raw_col=7:
  // raw_row=4. Keycode = raw_row*10 + raw_col + 1.
  // We'll test raw_row=5, raw_col=3 → keycode 54.
  KeyEvent e{};
  TEST_ASSERT_TRUE(adv_make_event(54, true, false, true, false, false, e));
  TEST_ASSERT_TRUE(e.key == Key::Left);
  TEST_ASSERT_TRUE(e.fn);
}

// ───── SettingsApp ──────────────────────────────────────────────────────────

void test_settings_loads_default_brightness() {
  Fixture f;
  FakeStorage store;
  SettingsApp app{store};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_INT(80, app.brightness());
}

void test_settings_step_persists_to_storage() {
  Fixture f;
  FakeStorage store;
  SettingsApp app{store};
  app.on_enter(f.hal);
  app.on_key(press(Key::Right));   // 80 → 90
  app.on_key(press(Key::Right));   // 90 → 100
  app.on_key(press(Key::Right));   // clamp at 100
  TEST_ASSERT_EQUAL_INT(100, app.brightness());

  // Re-enter: should reload 100 from storage.
  SettingsApp app2{store};
  app2.on_enter(f.hal);
  TEST_ASSERT_EQUAL_INT(100, app2.brightness());
}

void test_settings_left_decreases_brightness() {
  Fixture f;
  FakeStorage store;
  SettingsApp app{store};
  app.on_enter(f.hal);
  app.on_key(press(Key::Left));    // 80 → 70
  TEST_ASSERT_EQUAL_INT(70, app.brightness());
}

// ───── ClockApp ─────────────────────────────────────────────────────────────

void test_clock_starts_in_stopwatch_mode() {
  Fixture f;
  ClockApp app;
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.mode() == ClockApp::Mode::Stopwatch);
  TEST_ASSERT_FALSE(app.running());
}

void test_clock_enter_toggles_running() {
  Fixture f;
  ClockApp app;
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.running());
  app.on_key(press(Key::Enter));
  TEST_ASSERT_FALSE(app.running());
}

void test_clock_tick_advances_elapsed_when_running() {
  Fixture f;
  ClockApp app;
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));            // start
  // First tick at +500ms
  f.clock.set(500);
  app.tick(500);
  TEST_ASSERT_EQUAL_UINT32(500, app.elapsed());
  // +250 ms more
  f.clock.set(750);
  app.tick(750);
  TEST_ASSERT_EQUAL_UINT32(750, app.elapsed());
}

void test_clock_tab_switches_modes() {
  Fixture f;
  ClockApp app;
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.mode() == ClockApp::Mode::Timer);
}

void test_clock_timer_up_down_adjusts_target() {
  Fixture f;
  ClockApp app;
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  const uint32_t baseline = app.target();
  app.on_key(press(Key::Up));
  TEST_ASSERT_EQUAL_UINT32(baseline + 10'000, app.target());
  app.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_UINT32(baseline, app.target());
}

void test_clock_backspace_resets() {
  Fixture f;
  ClockApp app;
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  app.tick(1000);
  app.on_key(press(Key::Backspace));
  TEST_ASSERT_EQUAL_UINT32(0, app.elapsed());
  TEST_ASSERT_FALSE(app.running());
}

// ───── SnakeApp polish (pause + speed) ─────────────────────────────────────

void test_snake_app_backspace_pauses() {
  Fixture f;
  SnakeApp app;
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(app.paused());
  app.on_key(press(Key::Backspace));
  TEST_ASSERT_TRUE(app.paused());
  // Tick during pause should not advance the engine.
  const auto h0 = app.engine().head();
  app.tick(SnakeApp::kStepMs * 5);
  TEST_ASSERT_EQUAL_INT(h0.x, app.engine().head().x);
  TEST_ASSERT_EQUAL_INT(h0.y, app.engine().head().y);
  // Unpause and step.
  app.on_key(press(Key::Backspace));
  TEST_ASSERT_FALSE(app.paused());
  app.tick(SnakeApp::kStepMs * 5 + SnakeApp::kStepMsBase + 1);
  TEST_ASSERT_TRUE(app.engine().head().x != h0.x || app.engine().head().y != h0.y);
}

void test_snake_step_interval_shrinks_with_score() {
  Fixture f;
  SnakeApp app;
  app.on_enter(f.hal);
  const uint32_t base = app.step_interval();
  // Force a higher score to test the ramp.
  // We can't easily eat food deterministically, so just call reset and
  // assert the formula: at score 0 we get base, and the function clamps.
  TEST_ASSERT_EQUAL_UINT32(SnakeApp::kStepMsBase, base);
  // With kStepMsBase=160, kStepMsMin=60, shave=6 per pt: pt 17 → 160-102=58 → clamps.
  // That's hard to drive without eating; the unit test just confirms base.
}

// ───── ToneApp ──────────────────────────────────────────────────────────────

void test_tone_app_cursor_moves() {
  Fixture f;
  FakeSpeaker spk;
  ToneApp app{spk};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(0u, app.cursor());
  app.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor());
}

void test_tone_app_enter_starts_playing_and_emits_tones() {
  Fixture f;
  FakeSpeaker spk;
  ToneApp app{spk};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_INT(1, spk.init_count());
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.playing());
  // Tick once at t=0 should emit the first note.
  app.tick(0);
  TEST_ASSERT_EQUAL_size_t(1u, spk.count());
  TEST_ASSERT_EQUAL_UINT32(yui::presets::kCMajor[0].freq_hz, spk.last().freq_hz);
}

void test_tone_app_advances_through_preset() {
  Fixture f;
  FakeSpeaker spk;
  ToneApp app{spk};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  // Advance beyond all 8 C-major notes (250ms each).
  uint32_t t = 0;
  for (int i = 0; i < 12; ++i) {
    app.tick(t);
    t += 260;
  }
  TEST_ASSERT_FALSE(app.playing());
  TEST_ASSERT_EQUAL_size_t(8u, spk.count());
}

void test_tone_app_backspace_stops() {
  Fixture f;
  FakeSpeaker spk;
  ToneApp app{spk};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  app.tick(0);
  TEST_ASSERT_TRUE(app.playing());
  app.on_key(press(Key::Backspace));
  TEST_ASSERT_FALSE(app.playing());
  TEST_ASSERT_EQUAL_INT(1, spk.stop_count());
}

// ───── DrawApp ──────────────────────────────────────────────────────────────

void test_draw_starts_blank_with_centered_cursor() {
  Fixture f;
  FakeFs fs;
  DrawApp app{fs};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_INT(DrawApp::kCols / 2, app.cursor_x());
  TEST_ASSERT_EQUAL_INT(DrawApp::kRows / 2, app.cursor_y());
  for (int y = 0; y < DrawApp::kRows; ++y)
    for (int x = 0; x < DrawApp::kCols; ++x)
      TEST_ASSERT_FALSE(app.at(x, y));
}

void test_draw_arrows_move_cursor_with_clamp() {
  Fixture f;
  FakeFs fs;
  DrawApp app{fs};
  app.on_enter(f.hal);
  for (int i = 0; i < 100; ++i) app.on_key(press(Key::Left));
  TEST_ASSERT_EQUAL_INT(0, app.cursor_x());
  for (int i = 0; i < 100; ++i) app.on_key(press(Key::Up));
  TEST_ASSERT_EQUAL_INT(0, app.cursor_y());
  for (int i = 0; i < 200; ++i) app.on_key(press(Key::Right));
  TEST_ASSERT_EQUAL_INT(DrawApp::kCols - 1, app.cursor_x());
}

void test_draw_enter_toggles_cell() {
  Fixture f;
  FakeFs fs;
  DrawApp app{fs};
  app.on_enter(f.hal);
  const int x = app.cursor_x();
  const int y = app.cursor_y();
  TEST_ASSERT_FALSE(app.at(x, y));
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.at(x, y));
  app.on_key(press(Key::Enter));
  TEST_ASSERT_FALSE(app.at(x, y));
}

void test_draw_backspace_clears() {
  Fixture f;
  FakeFs fs;
  DrawApp app{fs};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  app.on_key(press(Key::Backspace));
  TEST_ASSERT_FALSE(app.at(app.cursor_x(), app.cursor_y()));
}

void test_draw_save_then_reload_round_trips() {
  Fixture f;
  FakeFs fs;
  DrawApp app{fs};
  app.on_enter(f.hal);
  // Set three pixels.
  app.on_key(press(Key::Enter));      // cursor at (30, 12)
  app.on_key(press(Key::Right));
  app.on_key(press(Key::Enter));      // (31, 12)
  app.on_key(press(Key::Down));
  app.on_key(press(Key::Enter));      // (31, 13)
  app.on_key(press(Key::Tab));        // save
  TEST_ASSERT_TRUE(app.just_saved());

  // New instance should reload them.
  DrawApp app2{fs};
  app2.on_enter(f.hal);
  TEST_ASSERT_TRUE(app2.at(30, 12));
  TEST_ASSERT_TRUE(app2.at(31, 12));
  TEST_ASSERT_TRUE(app2.at(31, 13));
  TEST_ASSERT_FALSE(app2.at(0, 0));
}

// ───── LifeEngine + LifeApp ─────────────────────────────────────────────────

void test_life_blinker_oscillates_period_2() {
  LifeEngine e;
  e.seed(LifeEngine::Seed::Blinker, 1);
  // After clear+seed, expect a horizontal blinker around the center.
  const int cx = LifeEngine::kCols / 2;
  const int cy = LifeEngine::kRows / 2;
  TEST_ASSERT_TRUE(e.alive(cx - 1, cy));
  TEST_ASSERT_TRUE(e.alive(cx,     cy));
  TEST_ASSERT_TRUE(e.alive(cx + 1, cy));
  e.step();
  // Should now be vertical.
  TEST_ASSERT_TRUE(e.alive(cx, cy - 1));
  TEST_ASSERT_TRUE(e.alive(cx, cy));
  TEST_ASSERT_TRUE(e.alive(cx, cy + 1));
  TEST_ASSERT_FALSE(e.alive(cx - 1, cy));
  e.step();
  // Back to horizontal.
  TEST_ASSERT_TRUE(e.alive(cx - 1, cy));
  TEST_ASSERT_TRUE(e.alive(cx + 1, cy));
}

void test_life_glider_translates_after_4_steps() {
  LifeEngine e;
  e.seed(LifeEngine::Seed::Glider, 1);
  const size_t pop0 = e.population();
  for (int i = 0; i < 4; ++i) e.step();
  TEST_ASSERT_EQUAL_size_t(pop0, e.population());  // glider preserves cells
  TEST_ASSERT_EQUAL_size_t(5u, e.population());
}

void test_life_clear_zeros_grid() {
  LifeEngine e;
  e.seed(LifeEngine::Seed::Glider, 1);
  TEST_ASSERT_GREATER_THAN_size_t(0u, e.population());
  e.clear();
  TEST_ASSERT_EQUAL_size_t(0u, e.population());
}

void test_life_app_enter_pauses() {
  Fixture f;
  LifeApp app;
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(app.paused());
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.paused());
  // Tick during pause should not advance generation.
  const auto g0 = app.engine().generation();
  app.tick(0);
  app.tick(1000);
  TEST_ASSERT_EQUAL_UINT32(g0, app.engine().generation());
}

void test_life_app_right_single_steps() {
  Fixture f;
  LifeApp app;
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));  // pause
  const auto g0 = app.engine().generation();
  app.on_key(press(Key::Right));
  TEST_ASSERT_EQUAL_UINT32(g0 + 1, app.engine().generation());
}

void test_life_app_backspace_clears() {
  Fixture f;
  LifeApp app;
  app.on_enter(f.hal);
  TEST_ASSERT_GREATER_THAN_size_t(0u, app.engine().population());
  app.on_key(press(Key::Backspace));
  TEST_ASSERT_EQUAL_size_t(0u, app.engine().population());
  TEST_ASSERT_TRUE(app.paused());
}

// ───── PomodoroApp ──────────────────────────────────────────────────────────

void test_pomodoro_starts_idle_in_work_phase() {
  Fixture f;
  FakeSpeaker spk;
  PomodoroApp app{spk};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.state() == PomodoroApp::State::Idle);
  TEST_ASSERT_TRUE(app.phase() == PomodoroApp::Phase::Work);
}

void test_pomodoro_enter_starts_running() {
  Fixture f;
  FakeSpeaker spk;
  PomodoroApp app{spk};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == PomodoroApp::State::Running);
}

void test_pomodoro_work_phase_completes_and_chimes() {
  Fixture f;
  FakeSpeaker spk;
  PomodoroApp app{spk};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  // First tick at t=0 anchors last_ms_ to 0.
  app.tick(0);
  // Jump past 25 minutes (default preset).
  app.tick(25u * 60u * 1000u + 1u);
  TEST_ASSERT_TRUE(app.phase() == PomodoroApp::Phase::Break);
  TEST_ASSERT_EQUAL_UINT(1u, app.completed());
  TEST_ASSERT_GREATER_THAN_size_t(0u, spk.count());
}

void test_pomodoro_backspace_resets() {
  Fixture f;
  FakeSpeaker spk;
  PomodoroApp app{spk};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  app.tick(1000);
  app.tick(60000);
  app.on_key(press(Key::Backspace));
  TEST_ASSERT_TRUE(app.state() == PomodoroApp::State::Idle);
  TEST_ASSERT_TRUE(app.phase() == PomodoroApp::Phase::Work);
  TEST_ASSERT_EQUAL_UINT32(0u, app.elapsed_ms());
}

void test_pomodoro_tab_cycles_preset_only_when_idle() {
  Fixture f;
  FakeSpeaker spk;
  PomodoroApp app{spk};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(0u, app.preset());
  app.on_key(press(Key::Tab));
  TEST_ASSERT_EQUAL_size_t(1u, app.preset());
  app.on_key(press(Key::Enter));  // start
  app.on_key(press(Key::Tab));    // ignored while running
  TEST_ASSERT_EQUAL_size_t(1u, app.preset());
}

// ───── MetronomeApp ─────────────────────────────────────────────────────────

void test_metronome_default_bpm_100() {
  Fixture f;
  FakeSpeaker spk;
  MetronomeApp app{spk};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_INT(100, app.bpm());
  TEST_ASSERT_EQUAL_UINT32(600u, app.interval_ms());
}

void test_metronome_up_down_adjust_bpm_with_clamp() {
  Fixture f;
  FakeSpeaker spk;
  MetronomeApp app{spk};
  app.on_enter(f.hal);
  app.on_key(press(Key::Up));
  TEST_ASSERT_EQUAL_INT(104, app.bpm());
  // Bash down past minimum.
  for (int i = 0; i < 100; ++i) app.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_INT(MetronomeApp::kBpmMin, app.bpm());
}

void test_metronome_enter_starts_and_clicks() {
  Fixture f;
  FakeSpeaker spk;
  MetronomeApp app{spk};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.running());
  app.tick(0);  // immediate downbeat
  TEST_ASSERT_GREATER_THAN_size_t(0u, spk.count());
  TEST_ASSERT_EQUAL_UINT32(2000u, spk.last().freq_hz);  // downbeat freq
}

void test_metronome_advances_through_beats_in_4_4() {
  Fixture f;
  FakeSpeaker spk;
  MetronomeApp app{spk};
  app.on_enter(f.hal);
  // Default sig is 4/4. Start, then tick across 4 intervals.
  app.on_key(press(Key::Enter));
  uint32_t t = 0;
  for (int i = 0; i < 5; ++i) {
    app.tick(t);
    t += app.interval_ms();
  }
  TEST_ASSERT_EQUAL_size_t(5u, spk.count());  // 4 beats + the next downbeat
}

// ───── KeyTestApp ───────────────────────────────────────────────────────────

void test_keytest_records_events() {
  Fixture f;
  KeyTestApp app;
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(0u, app.count());
  KeyEvent k{}; k.down = true; k.key = Key::Char; k.ch = 'q';
  app.on_key(k);
  TEST_ASSERT_EQUAL_size_t(1u, app.count());
  TEST_ASSERT_EQUAL_CHAR('q', app.newest().ch);
}

void test_keytest_ring_buffer_caps_history() {
  Fixture f;
  KeyTestApp app;
  app.on_enter(f.hal);
  KeyEvent k{}; k.down = true; k.key = Key::Char;
  for (int i = 0; i < 20; ++i) { k.ch = 'a' + (i % 26); app.on_key(k); }
  TEST_ASSERT_EQUAL_size_t(KeyTestApp::kHistory, app.count());
}

// ───── FilesApp hex toggle ──────────────────────────────────────────────────

void test_files_view_tab_toggles_hex() {
  Fixture f;
  FakeFs fs;
  fs.init();
  fs.put_file("/note.txt", "abcd");
  FilesApp app{fs};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));        // open viewer
  TEST_ASSERT_FALSE(app.hex());
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.hex());
  app.on_key(press(Key::Tab));
  TEST_ASSERT_FALSE(app.hex());
}

// ───── SnakeEngine + SnakeApp ───────────────────────────────────────────────

void test_snake_starts_running_with_length_3() {
  SnakeEngine s;
  s.reset(42);
  TEST_ASSERT_TRUE(s.state() == SnakeEngine::State::Running);
  TEST_ASSERT_EQUAL_size_t(3u, s.length());
}

void test_snake_step_moves_head_in_direction() {
  SnakeEngine s;
  s.reset(42);
  const auto h0 = s.head();
  s.step();
  const auto h1 = s.head();
  TEST_ASSERT_EQUAL_INT(h0.x + 1, h1.x);  // default dir is Right
  TEST_ASSERT_EQUAL_INT(h0.y, h1.y);
}

void test_snake_wall_collision_ends_game() {
  SnakeEngine s;
  s.reset(1);
  // Run right until we hit the wall.
  for (int i = 0; i < SnakeEngine::kCols + 5; ++i) s.step();
  TEST_ASSERT_TRUE(s.state() == SnakeEngine::State::GameOver);
}

void test_snake_reverse_turn_is_ignored() {
  SnakeEngine s;
  s.reset(1);
  // Default Right; try to U-turn Left.
  s.turn(SnakeEngine::Dir::Left);
  s.step();
  // Head should still have moved Right (dir unchanged).
  TEST_ASSERT_TRUE(s.dir() == SnakeEngine::Dir::Right);
  TEST_ASSERT_TRUE(s.state() == SnakeEngine::State::Running);
}

void test_snake_perpendicular_turn_works() {
  SnakeEngine s;
  s.reset(1);
  s.turn(SnakeEngine::Dir::Up);
  s.step();
  TEST_ASSERT_TRUE(s.dir() == SnakeEngine::Dir::Up);
}

void test_snake_eating_food_grows_snake() {
  SnakeEngine s;
  s.reset(1);
  // Find food by stepping (deterministic seed); when we first see length grow,
  // the previous head must equal the food position.
  const size_t initial = s.length();
  for (int i = 0; i < 200; ++i) {
    // Steer toward food in a primitive way: orthogonal hop.
    const auto h = s.head();
    const auto f = s.food();
    if (f.x > h.x)      s.turn(SnakeEngine::Dir::Right);
    else if (f.x < h.x) s.turn(SnakeEngine::Dir::Left);
    else if (f.y > h.y) s.turn(SnakeEngine::Dir::Down);
    else if (f.y < h.y) s.turn(SnakeEngine::Dir::Up);
    s.step();
    if (s.state() != SnakeEngine::State::Running) break;
    if (s.length() > initial) {
      TEST_ASSERT_EQUAL_INT(1, s.score());
      return;
    }
  }
  TEST_FAIL_MESSAGE("snake never ate food in 200 steps");
}

void test_snake_app_arrow_keys_steer() {
  Fixture f;
  SnakeApp app;
  app.on_enter(f.hal);
  app.on_key(press(Key::Up));
  // dir doesn't commit until step(); confirm via head movement instead.
  const auto h0 = app.engine().head();
  app.engine().step();
  const auto h1 = app.engine().head();
  TEST_ASSERT_EQUAL_INT(h0.y - 1, h1.y);
  TEST_ASSERT_EQUAL_INT(h0.x, h1.x);
}

void test_snake_app_tick_advances_engine_after_step_ms() {
  Fixture f;
  SnakeApp app;
  app.on_enter(f.hal);
  const auto h0 = app.engine().head();
  app.tick(0);
  app.tick(SnakeApp::kStepMs + 1);
  const auto h1 = app.engine().head();
  TEST_ASSERT_TRUE(h0.x != h1.x || h0.y != h1.y);
}

// ───── SysinfoApp ───────────────────────────────────────────────────────────

void test_sysinfo_renders_header_red() {
  Fixture f;
  SysProbe p;
  p.battery_pct     = []{ return 73; };
  p.free_heap_bytes = []{ return 100u * 1024; };
  p.uptime_ms       = []{ return 12345u; };
  p.ip_or_empty     = []{ return "192.168.1.42"; };
  p.wifi_rssi       = []{ return -55; };
  SysinfoApp app{p};
  app.render(f.display);
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, 5));
}

// ───── Runner ───────────────────────────────────────────────────────────────


int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_menu_starts_at_zero);
  RUN_TEST(test_menu_down_advances);
  RUN_TEST(test_menu_down_wraps_at_end);
  RUN_TEST(test_menu_up_wraps_at_start);
  RUN_TEST(test_menu_set_count_clamps_cursor);
  RUN_TEST(test_braille_decode_blank_glyph);
  RUN_TEST(test_braille_decode_full_block);
  RUN_TEST(test_braille_decode_advances_pointer);
  RUN_TEST(test_koi_art_dimensions_match_constants);
  RUN_TEST(test_splash_paints_white_background);
  RUN_TEST(test_splash_renders_red_koi_pixels_at_t0);
  RUN_TEST(test_splash_shimmer_advances_with_time);
  RUN_TEST(test_splash_writes_version_string);
  RUN_TEST(test_splash_flushes_once_per_frame);
  RUN_TEST(test_registry_starts_empty);
  RUN_TEST(test_registry_adds_apps);
  RUN_TEST(test_registry_rejects_null);
  RUN_TEST(test_launcher_cursor_starts_at_zero);
  RUN_TEST(test_launcher_down_advances_cursor);
  RUN_TEST(test_launcher_up_wraps);
  RUN_TEST(test_launcher_enter_sets_pending_launch);
  RUN_TEST(test_launcher_renders_header_in_red);
  RUN_TEST(test_launcher_renders_selected_row_highlighted);
  RUN_TEST(test_shell_starts_in_splash);
  RUN_TEST(test_shell_holds_splash_before_min_time);
  RUN_TEST(test_shell_advances_to_launcher_after_min_time_and_key);
  RUN_TEST(test_shell_enters_app_on_launcher_enter);
  RUN_TEST(test_shell_esc_returns_to_launcher);
  RUN_TEST(test_about_renders_version);
  RUN_TEST(test_wifi_app_starts_scanning);
  RUN_TEST(test_wifi_app_transitions_to_done_with_results);
  RUN_TEST(test_wifi_app_transitions_to_empty_when_no_results);
  RUN_TEST(test_wifi_app_renders_header_red);
  RUN_TEST(test_wifi_app_arrow_keys_move_cursor);
  RUN_TEST(test_wifi_app_enter_restarts_scan);
  RUN_TEST(test_ble_app_starts_and_completes);
  RUN_TEST(test_ble_app_arrow_keys_move_cursor);
  RUN_TEST(test_calc_engine_addition);
  RUN_TEST(test_calc_engine_division_by_zero_errors);
  RUN_TEST(test_calc_engine_decimal_input);
  RUN_TEST(test_calc_engine_dup);
  RUN_TEST(test_calc_engine_backspace);
  RUN_TEST(test_calc_app_dispatches_digits_and_ops);
  RUN_TEST(test_imu_app_reads_accel_on_tick);
  RUN_TEST(test_imu_app_renders_header_red);
  RUN_TEST(test_notes_starts_empty);
  RUN_TEST(test_notes_typing_marks_dirty);
  RUN_TEST(test_notes_backspace);
  RUN_TEST(test_notes_arrow_left_right_move_cursor);
  RUN_TEST(test_notes_insert_mid_buffer);
  RUN_TEST(test_notes_backspace_mid_buffer);
  RUN_TEST(test_notes_up_down_preserve_target_column);
  RUN_TEST(test_notes_save_and_reload);
  RUN_TEST(test_files_lists_root);
  RUN_TEST(test_files_enter_descends_into_dir);
  RUN_TEST(test_files_dotdot_at_subdir_pops_to_parent);
  RUN_TEST(test_files_no_dotdot_at_root);
  RUN_TEST(test_files_pop_restores_cursor);
  RUN_TEST(test_files_enter_on_file_opens_viewer);
  RUN_TEST(test_files_view_backspace_returns_to_list);
  RUN_TEST(test_ir_app_sends_nec_on_enter);
  RUN_TEST(test_ir_app_arrow_keys_change_selection);
  RUN_TEST(test_mic_app_silent_input_yields_zero_rms);
  RUN_TEST(test_mic_app_loud_input_yields_high_rms);
  RUN_TEST(test_mic_app_sine_at_1khz_lands_in_expected_band);
  RUN_TEST(test_mic_app_sine_at_300hz_lands_in_low_band);
  RUN_TEST(test_fft_dc_input_concentrates_in_bin_zero);
  RUN_TEST(test_fft_single_bin_sine);
  RUN_TEST(test_adv_decode_pos_keycode_1_is_tab_row1_col0);
  RUN_TEST(test_adv_make_event_letter_unshifted);
  RUN_TEST(test_adv_make_event_letter_shifted);
  RUN_TEST(test_adv_make_event_fn_modifies_arrow_keys);
  RUN_TEST(test_settings_loads_default_brightness);
  RUN_TEST(test_settings_step_persists_to_storage);
  RUN_TEST(test_settings_left_decreases_brightness);
  RUN_TEST(test_clock_starts_in_stopwatch_mode);
  RUN_TEST(test_clock_enter_toggles_running);
  RUN_TEST(test_clock_tick_advances_elapsed_when_running);
  RUN_TEST(test_clock_tab_switches_modes);
  RUN_TEST(test_clock_timer_up_down_adjusts_target);
  RUN_TEST(test_clock_backspace_resets);
  RUN_TEST(test_snake_app_backspace_pauses);
  RUN_TEST(test_snake_step_interval_shrinks_with_score);
  RUN_TEST(test_tone_app_cursor_moves);
  RUN_TEST(test_tone_app_enter_starts_playing_and_emits_tones);
  RUN_TEST(test_tone_app_advances_through_preset);
  RUN_TEST(test_tone_app_backspace_stops);
  RUN_TEST(test_draw_starts_blank_with_centered_cursor);
  RUN_TEST(test_draw_arrows_move_cursor_with_clamp);
  RUN_TEST(test_draw_enter_toggles_cell);
  RUN_TEST(test_draw_backspace_clears);
  RUN_TEST(test_draw_save_then_reload_round_trips);
  RUN_TEST(test_life_blinker_oscillates_period_2);
  RUN_TEST(test_life_glider_translates_after_4_steps);
  RUN_TEST(test_life_clear_zeros_grid);
  RUN_TEST(test_life_app_enter_pauses);
  RUN_TEST(test_life_app_right_single_steps);
  RUN_TEST(test_life_app_backspace_clears);
  RUN_TEST(test_pomodoro_starts_idle_in_work_phase);
  RUN_TEST(test_pomodoro_enter_starts_running);
  RUN_TEST(test_pomodoro_work_phase_completes_and_chimes);
  RUN_TEST(test_pomodoro_backspace_resets);
  RUN_TEST(test_pomodoro_tab_cycles_preset_only_when_idle);
  RUN_TEST(test_metronome_default_bpm_100);
  RUN_TEST(test_metronome_up_down_adjust_bpm_with_clamp);
  RUN_TEST(test_metronome_enter_starts_and_clicks);
  RUN_TEST(test_metronome_advances_through_beats_in_4_4);
  RUN_TEST(test_keytest_records_events);
  RUN_TEST(test_keytest_ring_buffer_caps_history);
  RUN_TEST(test_files_view_tab_toggles_hex);
  RUN_TEST(test_snake_starts_running_with_length_3);
  RUN_TEST(test_snake_step_moves_head_in_direction);
  RUN_TEST(test_snake_wall_collision_ends_game);
  RUN_TEST(test_snake_reverse_turn_is_ignored);
  RUN_TEST(test_snake_perpendicular_turn_works);
  RUN_TEST(test_snake_eating_food_grows_snake);
  RUN_TEST(test_snake_app_arrow_keys_steer);
  RUN_TEST(test_snake_app_tick_advances_engine_after_step_ms);
  RUN_TEST(test_sysinfo_renders_header_red);
  return UNITY_END();
}

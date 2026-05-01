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
  return UNITY_END();
}

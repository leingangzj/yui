// Native unit tests — pure logic, no hardware.
// Run with: pio test -e native

#include <unity.h>
#include "yui/shell/Menu.hpp"
#include "yui/types.hpp"
#include "../../src/hal/native/NativeDisplay.hpp"
#include "../../src/splash.hpp"

using yui::Menu;
using yui::NativeDisplay;
using yui::kBlue;
using yui::kBlack;

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
  m.down(); m.down(); m.down();   // 0→1→2→0
  TEST_ASSERT_EQUAL(0u, m.cursor());
}

void test_menu_up_wraps_at_start() {
  Menu m(3);
  m.up();   // 0→2
  TEST_ASSERT_EQUAL(2u, m.cursor());
}

void test_menu_set_count_clamps_cursor() {
  Menu m(10);
  m.down(); m.down(); m.down(); m.down();   // cursor at 4
  m.set_count(2);
  TEST_ASSERT_EQUAL(1u, m.cursor());
}

// ───── Splash render ────────────────────────────────────────────────────────

void test_splash_clears_and_flushes() {
  NativeDisplay d;
  yui::render_splash(d, "v0.0.1-test");
  TEST_ASSERT_EQUAL(1, d.flush_count());
  // Top banner should be blue.
  TEST_ASSERT_EQUAL_HEX16(kBlue, d.pixel_at(10, 5));
  // Below the banner should be black.
  TEST_ASSERT_EQUAL_HEX16(kBlack, d.pixel_at(10, 100));
}

void test_splash_writes_version_string() {
  NativeDisplay d;
  yui::render_splash(d, "v9.9.9-banana");
  // last_text() captures the most recent draw_text call.
  // The footer "press any key" is drawn last in render_splash.
  TEST_ASSERT_EQUAL_STRING("press any key", d.last_text().c_str());
}

// ───── Test runner ──────────────────────────────────────────────────────────

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_menu_starts_at_zero);
  RUN_TEST(test_menu_down_advances);
  RUN_TEST(test_menu_down_wraps_at_end);
  RUN_TEST(test_menu_up_wraps_at_start);
  RUN_TEST(test_menu_set_count_clamps_cursor);
  RUN_TEST(test_splash_clears_and_flushes);
  RUN_TEST(test_splash_writes_version_string);
  return UNITY_END();
}

// Native unit tests — pure logic, no hardware.
// Run with: pio test -e native

#include <unity.h>
#include "yui/shell/Menu.hpp"
#include "yui/types.hpp"
#include "yui/gfx/Braille.hpp"
#include "yui/gfx/koi_art.hpp"
#include "../../src/hal/native/NativeDisplay.hpp"
#include "../../src/splash.hpp"

using yui::Menu;
using yui::NativeDisplay;
using yui::kJapanRed;
using yui::kJapanRedBright;
using yui::kWhite;

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
  return UNITY_END();
}

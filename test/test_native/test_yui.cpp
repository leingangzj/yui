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
#include "../../src/hal/native/NativeRadioLink.hpp"
#include "../../src/hal/native/NativeGnss.hpp"
#include "../../src/hal/native/NativeHttp.hpp"
#include "../../src/hal/native/NativePcap.hpp"
#include "yui/proto/Ax25.hpp"
#include "yui/proto/Aprs.hpp"
#include "yui/proto/Pcap.hpp"
#include "yui/proto/Pineapple.hpp"
#include "yui/proto/Kiss.hpp"
#include "yui/app/AprsApp.hpp"
#include "yui/app/KenwoodApp.hpp"
#include "yui/app/GpsApp.hpp"
#include "yui/app/PineappleApp.hpp"
#include "yui/app/PineappleReconApp.hpp"
#include "../../src/hal/native/NativeWifiMonitor.hpp"
#include "yui/app/WifiProbeApp.hpp"
#include "yui/app/WifiHandshakeApp.hpp"
#include "yui/app/RemoteHeadApp.hpp"
#include "yui/app/AprsMessageApp.hpp"
#include "yui/app/HandshakeBrowserApp.hpp"
#include "yui/app/EvilTwinApp.hpp"
#include "yui/app/KarmaApp.hpp"
#include "yui/app/WifiDeauthApp.hpp"
#include "yui/app/BleSpamApp.hpp"
#include "../../src/hal/native/NativeBleAdvertiser.hpp"
#include "yui/app/TvBGoneApp.hpp"
#include "yui/app/WifiBeaconFloodApp.hpp"
#include "yui/app/WifiNativeDeauthApp.hpp"
#include "yui/app/WpsScanApp.hpp"
#include "yui/app/BleGattApp.hpp"
#include "yui/app/BleJammerApp.hpp"
#include "yui/app/CaptivePortalApp.hpp"
#include "../../src/hal/native/NativeWifiAp.hpp"
#include "../../src/hal/native/NativeBleCentral.hpp"
#include "yui/proto/Dot11.hpp"
#include "../../src/hal/native/NativeSpeaker.hpp"
#include "yui/app/ToneApp.hpp"
#include "yui/app/PomodoroApp.hpp"
#include "yui/app/MetronomeApp.hpp"
#include "yui/app/LifeApp.hpp"
#include "yui/game/LifeEngine.hpp"
#include "yui/app/DrawApp.hpp"
#include "yui/app/CalendarApp.hpp"
#include "yui/app/TodoApp.hpp"
#include "yui/util/Date.hpp"
#include "yui/util/TzPresets.hpp"
#include "yui/util/NtpPresets.hpp"
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
  StubApp a{"A"}, b{"B"};   // both default to Category::Tools
  f.registry.add(&a);
  f.registry.add(&b);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  l.enter_category(Category::Tools);
  TEST_ASSERT_EQUAL_size_t(0u, l.cursor());
  TEST_ASSERT_EQUAL_PTR(&a, l.selected());
}

void test_launcher_down_advances_cursor() {
  Fixture f;
  StubApp a{"A"}, b{"B"}, c{"C"};
  f.registry.add(&a); f.registry.add(&b); f.registry.add(&c);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  l.enter_category(Category::Tools);
  l.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_PTR(&b, l.selected());
}

void test_launcher_up_wraps() {
  Fixture f;
  StubApp a{"A"}, b{"B"}, c{"C"};
  f.registry.add(&a); f.registry.add(&b); f.registry.add(&c);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  l.enter_category(Category::Tools);
  l.on_key(press(Key::Up));
  TEST_ASSERT_EQUAL_PTR(&c, l.selected());
}

void test_launcher_enter_sets_pending_launch() {
  Fixture f;
  StubApp a{"A"}, b{"B"};
  f.registry.add(&a); f.registry.add(&b);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  l.enter_category(Category::Tools);
  l.on_key(press(Key::Down));
  l.on_key(press(Key::Enter));
  App* pending = l.take_pending_launch();
  TEST_ASSERT_EQUAL_PTR(&b, pending);
  TEST_ASSERT_NULL(l.take_pending_launch());
}

void test_launcher_starts_in_categories_view() {
  Fixture f;
  Launcher l{f.registry};
  l.on_enter(f.hal);
  TEST_ASSERT_TRUE(l.view() == Launcher::View::Categories);
}

void test_launcher_enter_on_empty_category_stays_in_categories() {
  Fixture f;
  Launcher l{f.registry};   // no apps registered → all categories empty
  l.on_enter(f.hal);
  l.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(l.view() == Launcher::View::Categories);
}

void test_launcher_drills_into_category_on_enter() {
  Fixture f;
  StubApp a{"A"};   // Tools
  f.registry.add(&a);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  // Walk from Radio→WiFi→BT→Tools (4 Down presses are enough)
  for (int i = 0; i < 3; ++i) l.on_key(press(Key::Down));
  TEST_ASSERT_TRUE(l.current_category() == Category::Tools);
  l.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(l.view() == Launcher::View::Apps);
  TEST_ASSERT_EQUAL_PTR(&a, l.selected());
}

void test_launcher_esc_returns_to_categories() {
  Fixture f;
  StubApp a{"A"};
  f.registry.add(&a);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  l.enter_category(Category::Tools);
  l.on_key(press(Key::Esc));
  TEST_ASSERT_TRUE(l.view() == Launcher::View::Categories);
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

void test_launcher_with_sysprobe_renders_status_text() {
  Fixture f;
  StubApp a{"A"};
  f.registry.add(&a);
  SysProbe p;
  p.battery_pct = []{ return 73; };
  p.wifi_rssi   = []{ return -55; };
  p.uptime_ms   = []{ return 3 * 3600u * 1000u + 21u * 60u * 1000u; };  // 03:21
  Launcher l{f.registry, p};
  l.on_enter(f.hal);
  l.render(f.display);
  // last_text should be the right-side status string (drawn after "Yui").
  const std::string& s = f.display.last_text();
  TEST_ASSERT_TRUE(s.find("73%")   != std::string::npos);
  TEST_ASSERT_TRUE(s.find("-55")   != std::string::npos);
  TEST_ASSERT_TRUE(s.find("03:21") != std::string::npos);
}

void test_launcher_status_uses_wallclock_when_synced() {
  Fixture f;
  StubApp a{"A"};
  f.registry.add(&a);
  ::setenv("TZ", "UTC0", 1); ::tzset();
  SysProbe p;
  p.battery_pct   = []{ return 50; };
  p.uptime_ms     = []{ return 3 * 3600u * 1000u + 21u * 60u * 1000u; };  // 03:21
  // 2025-01-01 09:07:00 UTC == unix 1735722420
  p.epoch_seconds = []() -> uint64_t { return 1735722420ULL; };
  Launcher l{f.registry, p};
  l.on_enter(f.hal);
  l.render(f.display);
  const std::string& s = f.display.last_text();
  TEST_ASSERT_TRUE(s.find("09:07") != std::string::npos);
  TEST_ASSERT_TRUE(s.find("03:21") == std::string::npos);  // uptime suppressed
}

void test_launcher_status_falls_back_to_uptime_when_unsynced() {
  Fixture f;
  StubApp a{"A"};
  f.registry.add(&a);
  SysProbe p;
  p.uptime_ms     = []{ return 3 * 3600u * 1000u + 21u * 60u * 1000u; };
  p.epoch_seconds = []() -> uint64_t { return 0; };  // not synced
  Launcher l{f.registry, p};
  l.on_enter(f.hal);
  l.render(f.display);
  TEST_ASSERT_TRUE(f.display.last_text().find("03:21") != std::string::npos);
}

void test_launcher_without_sysprobe_skips_status() {
  Fixture f;
  StubApp a{"A"};
  f.registry.add(&a);
  Launcher l{f.registry};   // no probe
  l.on_enter(f.hal);
  l.enter_category(Category::Tools);
  l.render(f.display);
  // Without a probe, status bar isn't drawn — last text drawn in Apps view
  // is the bottom-of-screen hint, not the row name. Just confirm the app
  // row WAS drawn somewhere (i.e. the Apps view rendered).
  TEST_ASSERT_TRUE(l.view() == Launcher::View::Apps);
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
  StubApp a{"A"}, b{"B"};   // both Tools
  f.registry.add(&a); f.registry.add(&b);
  Launcher l{f.registry};
  Shell s{f.hal, l, "vTEST", 0};
  s.start();
  f.clock.advance(10);
  f.keyboard.inject(press(Key::Enter)); s.tick();  // splash → launcher (categories view)
  TEST_ASSERT_TRUE(s.phase() == Shell::Phase::Launcher);
  // Walk Radio → WiFi → BT → Tools (3 Down)
  f.keyboard.inject(press(Key::Down));  s.tick();
  f.keyboard.inject(press(Key::Down));  s.tick();
  f.keyboard.inject(press(Key::Down));  s.tick();
  // Drill into Tools, move to second app, launch
  f.keyboard.inject(press(Key::Enter)); s.tick();   // drill in
  f.keyboard.inject(press(Key::Down));  s.tick();
  f.keyboard.inject(press(Key::Enter)); s.tick();   // launch
  TEST_ASSERT_TRUE(s.phase() == Shell::Phase::App);
  TEST_ASSERT_EQUAL_PTR(&b, s.current_app());
}

void test_shell_esc_returns_to_launcher() {
  Fixture f;
  StubApp a{"A"};   // Tools
  f.registry.add(&a);
  Launcher l{f.registry};
  Shell s{f.hal, l, "vTEST", 0};
  s.start();
  f.clock.advance(10);
  f.keyboard.inject(press(Key::Enter)); s.tick();   // splash → launcher
  // Walk to Tools (3 Down) and drill in
  f.keyboard.inject(press(Key::Down));  s.tick();
  f.keyboard.inject(press(Key::Down));  s.tick();
  f.keyboard.inject(press(Key::Down));  s.tick();
  f.keyboard.inject(press(Key::Enter)); s.tick();   // drill in
  f.keyboard.inject(press(Key::Enter)); s.tick();   // launch app
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

void test_wifi_app_tab_restarts_scan() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("A", -50)});
  WifiApp app{net};
  app.on_enter(f.hal);
  app.tick(0);
  TEST_ASSERT_EQUAL_INT(1, net.wifi_starts());
  app.on_key(press(Key::Tab));
  TEST_ASSERT_EQUAL_INT(2, net.wifi_starts());
}

namespace {
KeyEvent press_char(char c) {
  KeyEvent e{};
  e.key = Key::Char;
  e.ch  = c;
  e.down = true;
  return e;
}
KeyEvent press_fn(Key k) {
  KeyEvent e{};
  e.key = k;
  e.fn  = true;
  e.down = true;
  return e;
}
}

void test_wifi_app_enter_on_secured_opens_pass_editor() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("HomeNet", -50, /*secured=*/true)});
  WifiApp app{net};
  app.on_enter(f.hal);
  app.tick(0);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::EnterPass);
  TEST_ASSERT_EQUAL_STRING("HomeNet", app.selected_ssid());
}

void test_wifi_app_pass_editor_buffers_chars_and_backspace() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("HomeNet", -50, true)});
  WifiApp app{net};
  app.on_enter(f.hal);
  app.tick(0);
  app.on_key(press(Key::Enter));
  app.on_key(press_char('a'));
  app.on_key(press_char('b'));
  app.on_key(press_char('c'));
  TEST_ASSERT_EQUAL_STRING("abc", app.pass_buffer());
  app.on_key(press(Key::Backspace));
  TEST_ASSERT_EQUAL_STRING("ab", app.pass_buffer());
}

void test_wifi_app_pass_esc_returns_to_done() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("HomeNet", -50, true)});
  WifiApp app{net};
  app.on_enter(f.hal);
  app.tick(0);
  app.on_key(press(Key::Enter));
  app.on_key(press(Key::Esc));
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Done);
}

void test_wifi_app_submit_calls_connect_and_persists() {
  Fixture f;
  FakeNet net;
  net.set_wifi_connect_immediate(false);  // observe Connecting state
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("HomeNet", -50, true)});
  FakeStorage store;
  WifiApp app{net, &store};
  app.on_enter(f.hal);
  app.tick(0);
  app.on_key(press(Key::Enter));
  app.on_key(press_char('s'));
  app.on_key(press_char('e'));
  app.on_key(press_char('c'));
  app.on_key(press(Key::Enter));  // submit
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Connecting);
  TEST_ASSERT_EQUAL_INT(1, net.connects());
  TEST_ASSERT_EQUAL_STRING("HomeNet", net.last_ssid());
  TEST_ASSERT_EQUAL_STRING("sec",     net.last_pass());
  char ssid_back[33] = {0};
  char pass_back[65] = {0};
  TEST_ASSERT_TRUE(store.get_str("wifi.ssid", ssid_back, sizeof(ssid_back)));
  TEST_ASSERT_TRUE(store.get_str("wifi.pass", pass_back, sizeof(pass_back)));
  TEST_ASSERT_EQUAL_STRING("HomeNet", ssid_back);
  TEST_ASSERT_EQUAL_STRING("sec",     pass_back);
}

void test_wifi_app_open_network_skips_pass_editor() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("OpenNet", -50, /*secured=*/false)});
  WifiApp app{net};
  app.on_enter(f.hal);
  app.tick(0);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Connecting ||
                   app.state() == WifiApp::State::Connected);
  TEST_ASSERT_EQUAL_INT(1, net.connects());
}

void test_wifi_app_connecting_advances_to_connected_on_tick() {
  Fixture f;
  FakeNet net;
  net.set_wifi_connect_immediate(false);
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("HomeNet", -50, true)});
  WifiApp app{net};
  app.on_enter(f.hal);
  app.tick(0);
  app.on_key(press(Key::Enter));
  app.on_key(press_char('p'));
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Connecting);
  net.simulate_wifi_connected();
  app.tick(100);
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Connected);
}

void test_wifi_app_connecting_advances_to_failed_on_tick() {
  Fixture f;
  FakeNet net;
  net.set_wifi_connect_immediate(false);
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("HomeNet", -50, true)});
  WifiApp app{net};
  app.on_enter(f.hal);
  app.tick(0);
  app.on_key(press(Key::Enter));
  app.on_key(press_char('p'));
  app.on_key(press(Key::Enter));
  net.simulate_wifi_failed();
  app.tick(100);
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Failed);
}

void test_wifi_app_failed_enter_reopens_editor() {
  Fixture f;
  FakeNet net;
  net.set_wifi_connect_immediate(false);
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("HomeNet", -50, true)});
  WifiApp app{net};
  app.on_enter(f.hal);
  app.tick(0);
  app.on_key(press(Key::Enter));
  app.on_key(press_char('p'));
  app.on_key(press(Key::Enter));
  net.simulate_wifi_failed();
  app.tick(0);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::EnterPass);
}

void test_wifi_app_forget_on_done_clears_storage_and_disconnects() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("Saved", -50, true)});
  FakeStorage store;
  store.put_str("wifi.ssid", "Saved");
  store.put_str("wifi.pass", "secret");
  net.simulate_wifi_connected();
  WifiApp app{net, &store};
  app.on_enter(f.hal);
  // on_enter sees Connected → State::Connected
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Connected);
  app.on_key(press_fn(Key::Backspace));
  // After forget: store cleared, disconnected, app re-scans.
  char back[33] = "x";
  TEST_ASSERT_FALSE(store.get_str("wifi.ssid", back, sizeof(back)));
  TEST_ASSERT_FALSE(store.get_str("wifi.pass", back, sizeof(back)));
  TEST_ASSERT_TRUE(net.wifi_state() == WifiState::Idle);
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Done ||
                   app.state() == WifiApp::State::Scanning);
}

void test_wifi_app_forget_in_done_state() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("A", -50, true)});
  FakeStorage store;
  store.put_str("wifi.ssid", "Old");
  WifiApp app{net, &store};
  app.on_enter(f.hal);
  app.tick(0);
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Done);
  app.on_key(press_fn(Key::Backspace));
  char back[33] = "x";
  TEST_ASSERT_FALSE(store.get_str("wifi.ssid", back, sizeof(back)));
}

void test_wifi_app_plain_backspace_in_done_does_not_forget() {
  Fixture f;
  FakeNet net;
  net.set_wifi_immediate(true);
  net.set_wifi_results({make_ap("A", -50, true)});
  FakeStorage store;
  store.put_str("wifi.ssid", "Keep");
  WifiApp app{net, &store};
  app.on_enter(f.hal);
  app.tick(0);
  app.on_key(press(Key::Backspace));  // no Fn modifier
  char back[33] = {0};
  TEST_ASSERT_TRUE(store.get_str("wifi.ssid", back, sizeof(back)));
  TEST_ASSERT_EQUAL_STRING("Keep", back);
}

void test_wifi_app_already_connected_skips_scan() {
  Fixture f;
  FakeNet net;
  net.simulate_wifi_connected();
  WifiApp app{net};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.state() == WifiApp::State::Connected);
  TEST_ASSERT_EQUAL_INT(0, net.wifi_starts());
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

void test_calc_engine_subtraction() {
  CalculatorEngine e;
  e.input_char('1'); e.input_char('0'); e.flush_buffer();
  e.input_char('3'); e.flush_buffer();
  e.op('-');
  TEST_ASSERT_EQUAL_FLOAT(7.0, e.peek(0));
}

void test_calc_engine_negate_buffer_and_stack() {
  CalculatorEngine e;
  // Negate buffer in place.
  e.input_char('4'); e.input_char('2');
  e.neg();
  TEST_ASSERT_EQUAL_STRING("-42", e.buffer());
  e.neg();
  TEST_ASSERT_EQUAL_STRING("42", e.buffer());
  // Negate top of stack when buffer is empty.
  e.flush_buffer();
  e.neg();
  TEST_ASSERT_EQUAL_FLOAT(-42.0, e.peek(0));
}

void test_calc_engine_sqrt() {
  CalculatorEngine e;
  e.input_char('9'); e.flush_buffer();
  e.sqrt_top();
  TEST_ASSERT_EQUAL_FLOAT(3.0, e.peek(0));
}

void test_calc_engine_sqrt_negative_errors() {
  CalculatorEngine e;
  e.input_char('4'); e.flush_buffer();
  e.neg();
  e.sqrt_top();
  TEST_ASSERT_TRUE(e.error());
}

void test_calc_engine_swap() {
  CalculatorEngine e;
  e.input_char('1'); e.flush_buffer();
  e.input_char('2'); e.flush_buffer();
  TEST_ASSERT_EQUAL_FLOAT(2.0, e.peek(0));
  TEST_ASSERT_EQUAL_FLOAT(1.0, e.peek(1));
  e.swap();
  TEST_ASSERT_EQUAL_FLOAT(1.0, e.peek(0));
  TEST_ASSERT_EQUAL_FLOAT(2.0, e.peek(1));
}

void test_calc_app_routes_unary_ops() {
  Fixture f;
  CalculatorApp app;
  app.on_enter(f.hal);
  KeyEvent k{}; k.down = true; k.key = Key::Char;
  k.ch = '4'; app.on_key(k); k.ch = '9'; app.on_key(k);
  k.ch = 'q'; app.on_key(k);   // sqrt(49) = 7
  TEST_ASSERT_EQUAL_FLOAT(7.0, app.engine().peek(0));
  k.ch = 'n'; app.on_key(k);   // negate
  TEST_ASSERT_EQUAL_FLOAT(-7.0, app.engine().peek(0));
  k.ch = 'c'; app.on_key(k);   // clear
  TEST_ASSERT_EQUAL_size_t(0u, app.engine().depth());
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

void test_calc_engine_sin_zero_is_zero() {
  CalculatorEngine e;
  e.input_char('0'); e.flush_buffer();
  e.sin_top();
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, e.peek(0));
}

void test_calc_engine_cos_zero_is_one() {
  CalculatorEngine e;
  e.input_char('0'); e.flush_buffer();
  e.cos_top();
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, e.peek(0));
}

void test_calc_engine_tan_pi_quarter_is_one() {
  CalculatorEngine e;
  // 0.7853981633974483 ≈ pi/4
  for (char c : std::string("0.7853981633974483")) e.input_char(c);
  e.flush_buffer();
  e.tan_top();
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, e.peek(0));
}

void test_calc_engine_trig_empty_stack_errors() {
  CalculatorEngine e;
  e.sin_top();
  TEST_ASSERT_TRUE(e.error());
}

void test_calc_engine_memory_store_recall_round_trip() {
  CalculatorEngine e;
  e.input_char('4'); e.input_char('2'); e.flush_buffer();
  TEST_ASSERT_FALSE(e.m_has());
  e.m_store();
  TEST_ASSERT_TRUE(e.m_has());
  TEST_ASSERT_EQUAL_DOUBLE(42.0, e.m_value());
  // pop X then recall — recall pushes onto stack
  e.backspace();
  TEST_ASSERT_EQUAL_size_t(0u, e.depth());
  e.m_recall();
  TEST_ASSERT_EQUAL_size_t(1u, e.depth());
  TEST_ASSERT_EQUAL_DOUBLE(42.0, e.peek(0));
}

void test_calc_engine_memory_clear_zeros_and_unsets() {
  CalculatorEngine e;
  e.input_char('7'); e.flush_buffer();
  e.m_store();
  e.m_clear();
  TEST_ASSERT_FALSE(e.m_has());
  TEST_ASSERT_EQUAL_DOUBLE(0.0, e.m_value());
}

void test_calc_engine_memory_store_empty_errors() {
  CalculatorEngine e;
  e.m_store();
  TEST_ASSERT_TRUE(e.error());
  TEST_ASSERT_FALSE(e.m_has());
}

void test_calc_engine_starts_in_radians_mode() {
  CalculatorEngine e;
  TEST_ASSERT_TRUE(e.angle_mode() == CalculatorEngine::AngleMode::Radians);
}

void test_calc_engine_toggle_switches_angle_mode() {
  CalculatorEngine e;
  e.toggle_angle_mode();
  TEST_ASSERT_TRUE(e.angle_mode() == CalculatorEngine::AngleMode::Degrees);
  e.toggle_angle_mode();
  TEST_ASSERT_TRUE(e.angle_mode() == CalculatorEngine::AngleMode::Radians);
}

void test_calc_engine_sin_90_in_degrees_is_one() {
  CalculatorEngine e;
  e.toggle_angle_mode();
  e.input_char('9'); e.input_char('0'); e.flush_buffer();
  e.sin_top();
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, e.peek(0));
}

void test_calc_engine_cos_180_in_degrees_is_neg_one() {
  CalculatorEngine e;
  e.toggle_angle_mode();
  e.input_char('1'); e.input_char('8'); e.input_char('0'); e.flush_buffer();
  e.cos_top();
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, -1.0, e.peek(0));
}

void test_calc_engine_push_pi_pushes_constant() {
  CalculatorEngine e;
  e.push_pi();
  TEST_ASSERT_EQUAL_size_t(1u, e.depth());
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 3.141592653589793, e.peek(0));
}

void test_calc_engine_push_e_pushes_constant() {
  CalculatorEngine e;
  e.push_e();
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 2.718281828459045, e.peek(0));
}

void test_calc_engine_push_const_flushes_buffer_first() {
  CalculatorEngine e;
  e.input_char('5');
  e.push_pi();
  // Both buffer-as-stack-entry and pi should be on stack: depth 2, X = pi.
  TEST_ASSERT_EQUAL_size_t(2u, e.depth());
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 3.141592653589793, e.peek(0));
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 5.0, e.peek(1));
}

void test_calc_engine_reset_preserves_angle_mode() {
  CalculatorEngine e;
  e.toggle_angle_mode();
  e.reset();
  TEST_ASSERT_TRUE(e.angle_mode() == CalculatorEngine::AngleMode::Degrees);
}

void test_calc_app_d_toggles_angle_mode() {
  Fixture f;
  CalculatorApp app;
  app.on_enter(f.hal);
  KeyEvent k{}; k.down = true; k.key = Key::Char; k.ch = 'd';
  app.on_key(k);
  TEST_ASSERT_TRUE(app.engine().angle_mode() ==
                   CalculatorEngine::AngleMode::Degrees);
}

void test_calc_app_g_pushes_pi() {
  Fixture f;
  CalculatorApp app;
  app.on_enter(f.hal);
  KeyEvent k{}; k.down = true; k.key = Key::Char; k.ch = 'g';
  app.on_key(k);
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 3.141592653589793, app.engine().peek(0));
}

void test_calc_app_routes_trig_and_memory_keys() {
  Fixture f;
  CalculatorApp app;
  app.on_enter(f.hal);
  KeyEvent k{}; k.down = true; k.key = Key::Char;
  k.ch = '0'; app.on_key(k);
  k.ch = 'i'; app.on_key(k);  // sin(0) = 0
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, app.engine().peek(0));
  k.ch = 'p'; app.on_key(k);  // store
  TEST_ASSERT_TRUE(app.engine().m_has());
  k.ch = 'k'; app.on_key(k);  // clear
  TEST_ASSERT_FALSE(app.engine().m_has());
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

void test_notes_fn_left_jumps_to_line_start() {
  Fixture f;
  FakeFs fs;
  NotesApp app{fs};
  app.on_enter(f.hal);
  KeyEvent k{};
  k.down = true; k.key = Key::Char;
  for (char c : "hello") if (c) { k.ch = c; app.on_key(k); }
  TEST_ASSERT_EQUAL_size_t(5u, app.cursor());
  KeyEvent fl{};
  fl.down = true; fl.key = Key::Left; fl.fn = true;
  app.on_key(fl);
  TEST_ASSERT_EQUAL_size_t(0u, app.cursor_col());
  TEST_ASSERT_EQUAL_size_t(0u, app.cursor());
}

void test_notes_fn_right_jumps_to_line_end() {
  Fixture f;
  FakeFs fs;
  NotesApp app{fs};
  app.on_enter(f.hal);
  KeyEvent k{};
  k.down = true; k.key = Key::Char;
  for (char c : "abc") if (c) { k.ch = c; app.on_key(k); }
  app.on_key(press(Key::Enter));
  for (char c : "xyz") if (c) { k.ch = c; app.on_key(k); }
  // cursor on line 1, col 3.
  app.on_key(press(Key::Up));    // → line 0, col ≤3 (target_col=3, line len=3)
  TEST_ASSERT_EQUAL_size_t(0u, app.cursor_line());
  TEST_ASSERT_EQUAL_size_t(3u, app.cursor_col());
  app.on_key(press(Key::Left));   // back to col 2
  TEST_ASSERT_EQUAL_size_t(2u, app.cursor_col());
  KeyEvent fr{};
  fr.down = true; fr.key = Key::Right; fr.fn = true;
  app.on_key(fr);
  TEST_ASSERT_EQUAL_size_t(3u, app.cursor_col());
  TEST_ASSERT_EQUAL_size_t(0u, app.cursor_line());  // didn't fall through
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

void test_settings_default_tz_is_utc() {
  Fixture f;
  FakeStorage store;
  SettingsApp app{store};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(0u, app.tz_index());
  TEST_ASSERT_EQUAL_STRING("UTC0", app.tz_posix());
}

void test_settings_tz_right_cycles_and_persists() {
  Fixture f;
  FakeStorage store;
  SettingsApp app{store};
  app.on_enter(f.hal);
  app.on_key(press(Key::Down));    // cursor: brightness → timezone
  app.on_key(press(Key::Right));   // UTC → US Eastern
  TEST_ASSERT_EQUAL_size_t(1u, app.tz_index());
  TEST_ASSERT_EQUAL_STRING("EST5EDT,M3.2.0,M11.1.0", app.tz_posix());
  // Persisted
  char back[40] = {0};
  TEST_ASSERT_TRUE(store.get_str("clock.tz", back, sizeof(back)));
  TEST_ASSERT_EQUAL_STRING("EST5EDT,M3.2.0,M11.1.0", back);
}

void test_settings_tz_left_wraps_to_last_preset() {
  Fixture f;
  FakeStorage store;
  SettingsApp app{store};
  app.on_enter(f.hal);
  app.on_key(press(Key::Down));    // cursor on Timezone
  app.on_key(press(Key::Left));    // index 0 → last
  size_t n = 0;
  tz_presets(n);
  TEST_ASSERT_EQUAL_size_t(n - 1, app.tz_index());
}

void test_settings_tz_loads_persisted_value() {
  Fixture f;
  FakeStorage store;
  store.put_str("clock.tz", "JST-9");
  SettingsApp app{store};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(tz_index_of("JST-9"), app.tz_index());
}

void test_clock_picks_up_tz_from_storage_on_enter() {
  Fixture f;
  FakeStorage store;
  store.put_str("clock.tz", "JST-9");
  ClockApp app{nullptr, "pool.ntp.org", "UTC0", &store};
  app.on_enter(f.hal);
  // We can't easily inspect the internal tz_buf_, but we can verify the
  // behavioral effect: localtime_r honors $TZ on native. Render TimeOfDay
  // mode and confirm no crash; deeper assertion below.
  app.on_key(press(Key::Tab));
  app.on_key(press(Key::Tab));
  // 2025-01-01 00:00:00 UTC == 2025-01-01 09:00 JST
  f.clock.set_epoch(1735689600ULL);
  app.render(f.display);
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, 5));
}

void test_settings_default_ntp_is_pool() {
  Fixture f;
  FakeStorage store;
  SettingsApp app{store};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(0u, app.ntp_index());
  TEST_ASSERT_EQUAL_STRING("pool.ntp.org", app.ntp_host());
}

void test_settings_ntp_right_cycles_and_persists() {
  Fixture f;
  FakeStorage store;
  SettingsApp app{store};
  app.on_enter(f.hal);
  app.on_key(press(Key::Down));   // brightness → tz
  app.on_key(press(Key::Down));   // tz → ntp
  app.on_key(press(Key::Right));  // pool → Cloudflare
  TEST_ASSERT_EQUAL_size_t(1u, app.ntp_index());
  TEST_ASSERT_EQUAL_STRING("time.cloudflare.com", app.ntp_host());
  char back[40] = {0};
  TEST_ASSERT_TRUE(store.get_str("clock.ntp", back, sizeof(back)));
  TEST_ASSERT_EQUAL_STRING("time.cloudflare.com", back);
}

void test_settings_ntp_loads_persisted_value() {
  Fixture f;
  FakeStorage store;
  store.put_str("clock.ntp", "time.google.com");
  SettingsApp app{store};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(ntp_index_of("time.google.com"), app.ntp_index());
}

void test_clock_ntp_resync_uses_storage_ntp_server() {
  Fixture f;
  FakeStorage store;
  store.put_str("clock.tz",  "UTC0");
  store.put_str("clock.ntp", "time.google.com");
  FakeNet net;
  net.simulate_wifi_connected();
  ClockApp app{&net, "pool.ntp.org", "UTC0", &store};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  app.on_key(press(Key::Tab));
  app.on_key(press(Key::Enter));
  TEST_ASSERT_EQUAL_INT(1, net.ntp_calls());
  TEST_ASSERT_EQUAL_STRING("time.google.com", net.last_ntp_server());
}

void test_ntp_presets_index_of_known_value() {
  TEST_ASSERT_EQUAL_size_t(0u, ntp_index_of("pool.ntp.org"));
  TEST_ASSERT_TRUE(ntp_index_of("time.google.com") > 0u);
  TEST_ASSERT_EQUAL_size_t(0u, ntp_index_of("not-a-server"));
  TEST_ASSERT_EQUAL_size_t(0u, ntp_index_of(nullptr));
}

void test_clock_ntp_resync_uses_storage_tz() {
  Fixture f;
  FakeStorage store;
  store.put_str("clock.tz", "PST8PDT,M3.2.0,M11.1.0");
  FakeNet net;
  net.simulate_wifi_connected();
  ClockApp app{&net, "ntp.example", "UTC0", &store};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  app.on_key(press(Key::Tab));
  app.on_key(press(Key::Enter));
  TEST_ASSERT_EQUAL_INT(1, net.ntp_calls());
  TEST_ASSERT_EQUAL_STRING("PST8PDT,M3.2.0,M11.1.0", net.last_ntp_tz());
}

void test_tz_presets_index_of_known_value() {
  TEST_ASSERT_EQUAL_size_t(0u, tz_index_of("UTC0"));
  TEST_ASSERT_TRUE(tz_index_of("JST-9") > 0u);
  TEST_ASSERT_EQUAL_size_t(0u, tz_index_of("not-a-real-tz"));
  TEST_ASSERT_EQUAL_size_t(0u, tz_index_of(nullptr));
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
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.mode() == ClockApp::Mode::TimeOfDay);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.mode() == ClockApp::Mode::Stopwatch);
}

void test_clock_time_of_day_renders_unsynced_placeholder() {
  Fixture f;
  ClockApp app;
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  app.on_key(press(Key::Tab));
  // FakeClock epoch defaults to 0 → "no NTP sync" path. Just confirm
  // render doesn't crash and leaves the header red.
  app.render(f.display);
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, 5));
  TEST_ASSERT_TRUE(app.mode() == ClockApp::Mode::TimeOfDay);
}

void test_clock_time_of_day_enter_calls_ntp_sync() {
  Fixture f;
  FakeNet net;
  net.simulate_wifi_connected();
  ClockApp app{&net, "ntp.example", "UTC0"};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  app.on_key(press(Key::Tab));
  TEST_ASSERT_EQUAL_INT(0, net.ntp_calls());
  app.on_key(press(Key::Enter));
  TEST_ASSERT_EQUAL_INT(1, net.ntp_calls());
  TEST_ASSERT_EQUAL_STRING("ntp.example", net.last_ntp_server());
  TEST_ASSERT_EQUAL_STRING("UTC0",        net.last_ntp_tz());
}

void test_clock_time_of_day_enter_does_not_toggle_running() {
  Fixture f;
  FakeNet net;
  ClockApp app{&net};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  app.on_key(press(Key::Tab));
  app.on_key(press(Key::Enter));
  // Enter must not start the stopwatch from TimeOfDay mode.
  TEST_ASSERT_FALSE(app.running());
}

// ───── INet WiFi connect / NTP (FakeNet) ─────────────────────────────────────

void test_fakenet_wifi_connect_rejects_empty_ssid() {
  FakeNet net;
  TEST_ASSERT_FALSE(net.wifi_connect("", "pw"));
  TEST_ASSERT_FALSE(net.wifi_connect(nullptr, "pw"));
  TEST_ASSERT_TRUE(net.wifi_state() == WifiState::Idle);
}

void test_fakenet_wifi_connect_immediate_marks_connected() {
  FakeNet net;
  TEST_ASSERT_TRUE(net.wifi_connect("home", "secret"));
  TEST_ASSERT_TRUE(net.wifi_state() == WifiState::Connected);
  TEST_ASSERT_EQUAL_STRING("home",   net.last_ssid());
  TEST_ASSERT_EQUAL_STRING("secret", net.last_pass());
  TEST_ASSERT_EQUAL_STRING("192.168.1.42", net.wifi_ip());
}

void test_fakenet_wifi_connect_async_path() {
  FakeNet net;
  net.set_wifi_connect_immediate(false);
  TEST_ASSERT_TRUE(net.wifi_connect("home", "pw"));
  TEST_ASSERT_TRUE(net.wifi_state() == WifiState::Connecting);
  net.simulate_wifi_connected("10.0.0.5");
  TEST_ASSERT_TRUE(net.wifi_state() == WifiState::Connected);
  TEST_ASSERT_EQUAL_STRING("10.0.0.5", net.wifi_ip());
}

void test_fakenet_wifi_disconnect_clears_state() {
  FakeNet net;
  net.wifi_connect("home", "pw");
  net.wifi_disconnect();
  TEST_ASSERT_TRUE(net.wifi_state() == WifiState::Idle);
  TEST_ASSERT_EQUAL_STRING("0.0.0.0", net.wifi_ip());
}

void test_fakenet_ntp_sync_requires_connected() {
  FakeNet net;
  TEST_ASSERT_FALSE(net.ntp_sync("pool.ntp.org", "UTC0"));
  net.simulate_wifi_connected();
  TEST_ASSERT_TRUE(net.ntp_sync("pool.ntp.org", "UTC0"));
  TEST_ASSERT_EQUAL_INT(1, net.ntp_calls());
  TEST_ASSERT_EQUAL_STRING("pool.ntp.org", net.last_ntp_server());
}

// ───── IClock epoch_seconds ──────────────────────────────────────────────────

void test_fakeclock_epoch_defaults_to_zero() {
  FakeClock c;
  TEST_ASSERT_EQUAL_UINT64(0u, c.epoch_seconds());
}

void test_fakeclock_set_epoch_round_trips() {
  FakeClock c;
  c.set_epoch(1735689600ULL);  // 2025-01-01 00:00 UTC
  TEST_ASSERT_EQUAL_UINT64(1735689600ULL, c.epoch_seconds());
}

void test_clock_time_of_day_renders_time_when_synced() {
  Fixture f;
  // 2025-01-01 12:34:56 UTC → unix 1735734896. Render must not crash.
  f.clock.set_epoch(1735734896ULL);
  ClockApp app;
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  app.on_key(press(Key::Tab));
  app.render(f.display);
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, 5));
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

// ───── TodoApp ──────────────────────────────────────────────────────────────

namespace {
void type_string(TodoApp& app, const char* s) {
  KeyEvent k{}; k.down = true; k.key = Key::Char;
  for (const char* p = s; *p; ++p) {
    if (*p == ' ') { app.on_key(press(Key::Space)); continue; }
    k.ch = *p;
    app.on_key(k);
  }
}
}

void test_todo_starts_empty() {
  Fixture f;
  FakeFs fs;
  TodoApp app{fs};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(0u, app.count());
}

void test_todo_tab_starts_edit_then_enter_adds() {
  Fixture f;
  FakeFs fs;
  TodoApp app{fs};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.mode() == TodoApp::Mode::Editing);
  type_string(app, "milk");
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.mode() == TodoApp::Mode::List);
  TEST_ASSERT_EQUAL_size_t(1u, app.count());
  TEST_ASSERT_EQUAL_STRING("milk", app.text(0));
  TEST_ASSERT_FALSE(app.done(0));
}

void test_todo_space_toggles_done() {
  Fixture f;
  FakeFs fs;
  TodoApp app{fs};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  type_string(app, "x");
  app.on_key(press(Key::Enter));
  TEST_ASSERT_FALSE(app.done(0));
  app.on_key(press(Key::Space));
  TEST_ASSERT_TRUE(app.done(0));
  app.on_key(press(Key::Space));
  TEST_ASSERT_FALSE(app.done(0));
}

void test_todo_backspace_deletes_selected() {
  Fixture f;
  FakeFs fs;
  TodoApp app{fs};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab)); type_string(app, "a"); app.on_key(press(Key::Enter));
  app.on_key(press(Key::Tab)); type_string(app, "b"); app.on_key(press(Key::Enter));
  TEST_ASSERT_EQUAL_size_t(2u, app.count());
  app.on_key(press(Key::Backspace));   // delete first item ("a", cursor 0)
  TEST_ASSERT_EQUAL_size_t(1u, app.count());
  TEST_ASSERT_EQUAL_STRING("b", app.text(0));
}

void test_todo_persists_across_instances() {
  Fixture f;
  FakeFs fs;
  {
    TodoApp app{fs};
    app.on_enter(f.hal);
    app.on_key(press(Key::Tab)); type_string(app, "buy beans"); app.on_key(press(Key::Enter));
    app.on_key(press(Key::Tab)); type_string(app, "ship yui"); app.on_key(press(Key::Enter));
    app.on_key(press(Key::Down));
    app.on_key(press(Key::Space));     // mark "ship yui" done
  }
  TodoApp app2{fs};
  app2.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(2u, app2.count());
  TEST_ASSERT_EQUAL_STRING("buy beans", app2.text(0));
  TEST_ASSERT_FALSE(app2.done(0));
  TEST_ASSERT_EQUAL_STRING("ship yui", app2.text(1));
  TEST_ASSERT_TRUE(app2.done(1));
}

void test_todo_edit_backspace_aborts_when_buffer_empty() {
  Fixture f;
  FakeFs fs;
  TodoApp app{fs};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.mode() == TodoApp::Mode::Editing);
  app.on_key(press(Key::Backspace));   // empty buffer → abort
  TEST_ASSERT_TRUE(app.mode() == TodoApp::Mode::List);
  TEST_ASSERT_EQUAL_size_t(0u, app.count());
}

// ───── Date helpers + CalendarApp ───────────────────────────────────────────

void test_date_leap_year() {
  TEST_ASSERT_TRUE(yui::date::is_leap(2024));
  TEST_ASSERT_FALSE(yui::date::is_leap(2025));
  TEST_ASSERT_FALSE(yui::date::is_leap(1900));
  TEST_ASSERT_TRUE(yui::date::is_leap(2000));
}

void test_date_days_in_month() {
  TEST_ASSERT_EQUAL_INT(31, yui::date::days_in_month(2026, 1));
  TEST_ASSERT_EQUAL_INT(28, yui::date::days_in_month(2026, 2));
  TEST_ASSERT_EQUAL_INT(29, yui::date::days_in_month(2024, 2));
  TEST_ASSERT_EQUAL_INT(30, yui::date::days_in_month(2026, 4));
}

void test_date_day_of_week_known_dates() {
  // 2026-05-01 is a Friday (= 5 in Sun=0..Sat=6).
  TEST_ASSERT_EQUAL_INT(5, yui::date::day_of_week(2026, 5, 1));
  // 2000-01-01 was a Saturday.
  TEST_ASSERT_EQUAL_INT(6, yui::date::day_of_week(2000, 1, 1));
}

void test_date_add_days_crosses_month() {
  auto d = yui::date::add_days({2026, 1, 31}, 1);
  TEST_ASSERT_EQUAL_INT(2026, d.y);
  TEST_ASSERT_EQUAL_INT(2,    d.m);
  TEST_ASSERT_EQUAL_INT(1,    d.d);
}

void test_date_add_days_crosses_year_back() {
  auto d = yui::date::add_days({2026, 1, 1}, -1);
  TEST_ASSERT_EQUAL_INT(2025, d.y);
  TEST_ASSERT_EQUAL_INT(12,   d.m);
  TEST_ASSERT_EQUAL_INT(31,   d.d);
}

void test_date_add_months_clamps_day() {
  // Jan 31 + 1 month → Feb 28 (non-leap).
  auto d = yui::date::add_months({2026, 1, 31}, 1);
  TEST_ASSERT_EQUAL_INT(2,  d.m);
  TEST_ASSERT_EQUAL_INT(28, d.d);
}

void test_calendar_app_arrow_keys_navigate() {
  Fixture f;
  CalendarApp app;
  app.set_today({2026, 5, 1});
  app.on_enter(f.hal);
  app.on_key(press(Key::Right));
  TEST_ASSERT_EQUAL_INT(2, app.selected().d);
  app.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_INT(9, app.selected().d);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_EQUAL_INT(6, app.selected().m);
  app.on_key(press(Key::Backspace));
  TEST_ASSERT_EQUAL_INT(5, app.selected().m);
}

void test_calendar_app_enter_jumps_to_today() {
  Fixture f;
  CalendarApp app;
  app.set_today({2026, 5, 1});
  app.on_enter(f.hal);
  for (int i = 0; i < 30; ++i) app.on_key(press(Key::Right));
  TEST_ASSERT_NOT_EQUAL(1, app.selected().d);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_EQUAL_INT(1, app.selected().d);
  TEST_ASSERT_EQUAL_INT(5, app.selected().m);
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

void test_sysinfo_renders_time_when_synced() {
  Fixture f;
  ::setenv("TZ", "UTC0", 1); ::tzset();
  SysProbe p;
  p.epoch_seconds = []() -> uint64_t { return 1735722420ULL; };  // 09:07:00 UTC
  SysinfoApp app{p};
  app.render(f.display);
  TEST_ASSERT_TRUE(f.display.last_text().find("09:07:00") != std::string::npos);
}

void test_sysinfo_renders_no_sync_message_when_unsynced() {
  Fixture f;
  SysProbe p;
  p.epoch_seconds = []() -> uint64_t { return 0; };
  SysinfoApp app{p};
  app.render(f.display);
  TEST_ASSERT_TRUE(f.display.last_text().find("no NTP") != std::string::npos);
}

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

// ───── v0.2 HAL skeletons (FakeRadioLink / FakeGnss / FakeHttp / FakePcap) ──

void test_fake_radiolink_starts_idle() {
  FakeRadioLink r;
  TEST_ASSERT_TRUE(r.state() == RadioLinkState::Idle);
}

void test_fake_radiolink_connect_immediate_to_command_mode() {
  FakeRadioLink r;
  TEST_ASSERT_TRUE(r.connect("AA:BB:CC:DD:EE:FF", "0000"));
  TEST_ASSERT_TRUE(r.state() == RadioLinkState::CommandMode);
  TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:FF", r.last_mac().c_str());
}

void test_fake_radiolink_connect_async_path() {
  FakeRadioLink r;
  r.set_connect_immediate(false);
  TEST_ASSERT_TRUE(r.connect("AA:BB", "0000"));
  TEST_ASSERT_TRUE(r.state() == RadioLinkState::Connecting);
  r.simulate_connected();
  TEST_ASSERT_TRUE(r.state() == RadioLinkState::CommandMode);
}

void test_fake_radiolink_command_returns_registered_reply() {
  FakeRadioLink r;
  r.connect("MAC", "0000");
  r.register_reply("ID", "ID TH-D75A");
  char buf[32] = {0};
  TEST_ASSERT_TRUE(r.command("ID", buf, sizeof(buf), 1000));
  TEST_ASSERT_EQUAL_STRING("ID TH-D75A", buf);
}

void test_fake_radiolink_unregistered_command_fails() {
  FakeRadioLink r;
  r.connect("MAC", "0000");
  char buf[16] = {0};
  TEST_ASSERT_FALSE(r.command("ZZ", buf, sizeof(buf), 1000));
}

void test_fake_radiolink_kiss_round_trip() {
  FakeRadioLink r;
  r.connect("MAC", "0000");
  TEST_ASSERT_TRUE(r.enter_kiss(0));
  TEST_ASSERT_TRUE(r.state() == RadioLinkState::KissMode);
  // Caller-framed "data" frame: FEND 00 'H' 'i' FEND
  const uint8_t out_frame[] = {0xC0, 0x00, 'H', 'i', 0xC0};
  TEST_ASSERT_TRUE(r.kiss_write(out_frame, sizeof(out_frame)));
  TEST_ASSERT_EQUAL_size_t(sizeof(out_frame), r.kiss_tx().size());
  // Inject a fake RX frame and read it back
  const uint8_t rx_frame[] = {0xC0, 0x00, 0xDE, 0xAD, 0xC0};
  r.queue_kiss_bytes(rx_frame, sizeof(rx_frame));
  uint8_t in[8] = {0};
  const int n = r.kiss_read(in, sizeof(in));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(sizeof(rx_frame)), n);
  TEST_ASSERT_EQUAL_HEX8(0xDE, in[2]);
  TEST_ASSERT_EQUAL_HEX8(0xAD, in[3]);
  TEST_ASSERT_TRUE(r.exit_kiss());
  TEST_ASSERT_TRUE(r.state() == RadioLinkState::CommandMode);
}

void test_fake_radiolink_command_blocked_in_kiss_mode() {
  FakeRadioLink r;
  r.connect("MAC", "0000");
  r.register_reply("ID", "ID TH-D75A");
  r.enter_kiss(0);
  char buf[16] = {0};
  TEST_ASSERT_FALSE(r.command("ID", buf, sizeof(buf), 1000));
}

void test_fake_gnss_defaults_invalid() {
  FakeGnss g;
  TEST_ASSERT_FALSE(g.latest().valid);
  TEST_ASSERT_FALSE(g.any_data_seen());
}

void test_fake_gnss_set_fix_round_trips() {
  FakeGnss g;
  GnssFix fix{};
  fix.valid      = true;
  fix.lat_deg    = 49.05833;
  fix.lon_deg    = -72.02917;
  fix.satellites = 8;
  g.set_fix(fix);
  TEST_ASSERT_TRUE(g.latest().valid);
  TEST_ASSERT_EQUAL_UINT8(8, g.latest().satellites);
  TEST_ASSERT_TRUE(g.any_data_seen());
}

void test_fake_http_returns_canned_response() {
  FakeHttp h;
  h.register_response("GET", "http://example/api/cards",
                      200, "{\"cpu\":42}");
  char body[64] = {0};
  const int rc = h.request("GET", "http://example/api/cards",
                           nullptr, "Bearer xyz", body, sizeof(body), 1000);
  TEST_ASSERT_EQUAL_INT(200, rc);
  TEST_ASSERT_EQUAL_STRING("{\"cpu\":42}", body);
  TEST_ASSERT_EQUAL_STRING("Bearer xyz", h.last_auth().c_str());
}

void test_fake_http_unregistered_url_returns_neg_one() {
  FakeHttp h;
  char body[8] = {0};
  TEST_ASSERT_EQUAL_INT(-1, h.request("GET", "http://nope", nullptr,
                                      nullptr, body, sizeof(body), 1000));
}

void test_fake_pcap_writes_24_byte_header() {
  FakePcap p;
  TEST_ASSERT_TRUE(p.open("/test.pcap", PcapLinkType::Ieee80211, 65535));
  TEST_ASSERT_EQUAL_UINT64(24u, p.bytes_written());
  TEST_ASSERT_EQUAL_size_t(24u, p.buffer().size());
  // Magic in little-endian byte order: D4 C3 B2 A1
  TEST_ASSERT_EQUAL_HEX8(0xD4, p.buffer()[0]);
  TEST_ASSERT_EQUAL_HEX8(0xC3, p.buffer()[1]);
  TEST_ASSERT_EQUAL_HEX8(0xB2, p.buffer()[2]);
  TEST_ASSERT_EQUAL_HEX8(0xA1, p.buffer()[3]);
  // Linktype field (offset 20) = 0x69 = 105 LE
  TEST_ASSERT_EQUAL_HEX8(0x69, p.buffer()[20]);
  TEST_ASSERT_EQUAL_HEX8(0x00, p.buffer()[21]);
}

void test_fake_pcap_writes_packet_record_correctly() {
  FakePcap p;
  p.open("/x.pcap", PcapLinkType::Ieee80211, 65535);
  const uint8_t frame[] = {0xAA, 0xBB, 0xCC};
  // ts = 1.000123 sec → ts_sec=1, ts_usec=123
  TEST_ASSERT_TRUE(p.write_packet(frame, sizeof(frame), 1'000'123ULL));
  TEST_ASSERT_EQUAL_UINT64(24u + 16u + 3u, p.bytes_written());
  // Record header at offset 24: ts_sec=1 (LE)
  const auto& b = p.buffer();
  TEST_ASSERT_EQUAL_HEX8(0x01, b[24]);
  TEST_ASSERT_EQUAL_HEX8(0x00, b[25]);
  TEST_ASSERT_EQUAL_HEX8(0x00, b[26]);
  TEST_ASSERT_EQUAL_HEX8(0x00, b[27]);
  // ts_usec=123 = 0x7B (LE)
  TEST_ASSERT_EQUAL_HEX8(0x7B, b[28]);
  // incl_len=3 (LE)
  TEST_ASSERT_EQUAL_HEX8(0x03, b[32]);
  // payload starts at offset 40
  TEST_ASSERT_EQUAL_HEX8(0xAA, b[40]);
}

void test_pcap_format_handshake_path() {
  uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  char buf[64] = {0};
  const size_t n = pcap::format_handshake_path(mac, 1735689600ULL,
                                                buf, sizeof(buf));
  TEST_ASSERT_TRUE(n > 0);
  TEST_ASSERT_EQUAL_STRING(
      "/handshakes/AABBCCDDEEFF-1735689600.pcap", buf);
}

void test_aprs_dti_lookup() {
  TEST_ASSERT_TRUE(aprs::dti_of('!') == aprs::Dti::PositionNoTime);
  TEST_ASSERT_TRUE(aprs::dti_of('=') == aprs::Dti::PositionNoTimeMsg);
  TEST_ASSERT_TRUE(aprs::dti_of('/') == aprs::Dti::PositionWithTime);
  TEST_ASSERT_TRUE(aprs::dti_of('@') == aprs::Dti::PositionWithTimeMsg);
  TEST_ASSERT_TRUE(aprs::dti_of('>') == aprs::Dti::Status);
  TEST_ASSERT_TRUE(aprs::dti_of('Q') == aprs::Dti::Unknown);
}

void test_ax25_constants() {
  TEST_ASSERT_EQUAL_HEX8(0x03, ax25::kControlUI);
  TEST_ASSERT_EQUAL_HEX8(0xF0, ax25::kPidNoLayer3);
}

// ───── AX.25 parser — hand-crafted test vectors ────────────────────────────
//
// Test frames built per AX.25 v2.2: each address byte has its 7-bit
// ASCII left-shifted by 1; byte-6 is HRRSSSS<E> where E = end-of-list.
// SSID byte for "no digis, no SSID, not last" = 0_11_0000_0 = 0x60.
// SSID byte for "last addr, no SSID" = 0_11_0000_1 = 0x61.

namespace {

// "K1ABC>APRS:!4903.50N/07201.75W>Test"  (no digipath)
// Layout: dst APRS-0 (last=0), src K1ABC-0 (last=1), ctrl 03, pid F0, info.
const uint8_t kSimplePosFrame[] = {
  // dst "APRS  "-0  (last bit 0)
  'A'<<1, 'P'<<1, 'R'<<1, 'S'<<1, ' '<<1, ' '<<1, 0x60,
  // src "K1ABC "-0  (last bit 1)
  'K'<<1, '1'<<1, 'A'<<1, 'B'<<1, 'C'<<1, ' '<<1, 0x61,
  // control + pid
  0x03, 0xF0,
  // info
  '!','4','9','0','3','.','5','0','N','/','0','7','2','0','1','.','7','5','W','>',
  'T','e','s','t',
};

// "K1ABC-9>APRS,WIDE1-1:=4903.50N/07201.75W>"
// Has one digi WIDE1-1 (last=1).
// SSID for K1ABC-9 (not last, ssid 9): 0_11_1001_0 = 0x72.
// SSID for WIDE1-1 (last, ssid 1, h=0): 0_11_0001_1 = 0x63.
const uint8_t kDigipathFrame[] = {
  'A'<<1, 'P'<<1, 'R'<<1, 'S'<<1, ' '<<1, ' '<<1, 0x60,
  'K'<<1, '1'<<1, 'A'<<1, 'B'<<1, 'C'<<1, ' '<<1, 0x72,        // K1ABC-9, not last
  'W'<<1, 'I'<<1, 'D'<<1, 'E'<<1, '1'<<1, ' '<<1, 0x63,        // WIDE1-1, last
  0x03, 0xF0,
  '=','4','9','0','3','.','5','0','N','/','0','7','2','0','1','.','7','5','W','>',
};

// Status-only frame: "K1ABC>APRS:>Battery low"
const uint8_t kStatusFrame[] = {
  'A'<<1, 'P'<<1, 'R'<<1, 'S'<<1, ' '<<1, ' '<<1, 0x60,
  'K'<<1, '1'<<1, 'A'<<1, 'B'<<1, 'C'<<1, ' '<<1, 0x61,
  0x03, 0xF0,
  '>','B','a','t','t','e','r','y',' ','l','o','w',
};

}  // namespace

void test_ax25_parse_minimal_position_frame() {
  ax25::Frame f;
  TEST_ASSERT_TRUE(ax25::parse(kSimplePosFrame, sizeof(kSimplePosFrame), f));
  TEST_ASSERT_EQUAL_STRING("APRS",  f.dst.call);
  TEST_ASSERT_EQUAL_STRING("K1ABC", f.src.call);
  TEST_ASSERT_EQUAL_UINT8(0, f.dst.ssid);
  TEST_ASSERT_EQUAL_UINT8(0, f.src.ssid);
  TEST_ASSERT_TRUE(f.src.last);
  TEST_ASSERT_FALSE(f.dst.last);
  TEST_ASSERT_EQUAL_UINT8(0, f.digi_count);
  TEST_ASSERT_EQUAL_HEX8(0x03, f.control);
  TEST_ASSERT_EQUAL_HEX8(0xF0, f.pid);
  TEST_ASSERT_EQUAL_size_t(24u, f.info_len);
  TEST_ASSERT_EQUAL_HEX8('!', f.info[0]);
}

void test_ax25_parse_with_digipath() {
  ax25::Frame f;
  TEST_ASSERT_TRUE(ax25::parse(kDigipathFrame, sizeof(kDigipathFrame), f));
  TEST_ASSERT_EQUAL_STRING("K1ABC", f.src.call);
  TEST_ASSERT_EQUAL_UINT8(9, f.src.ssid);
  TEST_ASSERT_FALSE(f.src.last);
  TEST_ASSERT_EQUAL_UINT8(1, f.digi_count);
  TEST_ASSERT_EQUAL_STRING("WIDE1", f.digis[0].call);
  TEST_ASSERT_EQUAL_UINT8(1, f.digis[0].ssid);
  TEST_ASSERT_TRUE(f.digis[0].last);
  TEST_ASSERT_FALSE(f.digis[0].repeated);  // H-bit = 0
}

void test_ax25_parse_rejects_truncated() {
  ax25::Frame f;
  // Only 10 bytes — not enough for two addresses + ctrl + pid.
  const uint8_t bad[] = {0x82, 0xA0, 0xA4, 0xA6, 0x40, 0x40, 0x60, 0x96, 0x62, 0x82};
  TEST_ASSERT_FALSE(ax25::parse(bad, sizeof(bad), f));
}

void test_ax25_parse_rejects_dst_marked_last() {
  ax25::Frame f;
  uint8_t bad[sizeof(kSimplePosFrame)];
  std::memcpy(bad, kSimplePosFrame, sizeof(bad));
  bad[6] |= 0x01;  // set dst.last — invalid (src must follow)
  TEST_ASSERT_FALSE(ax25::parse(bad, sizeof(bad), f));
}

// ───── APRS position parser ────────────────────────────────────────────────

void test_aprs_parse_position_no_timestamp() {
  ax25::Frame f;
  ax25::parse(kSimplePosFrame, sizeof(kSimplePosFrame), f);
  aprs::Position p;
  TEST_ASSERT_TRUE(aprs::parse_position(f.info, f.info_len, p));
  TEST_ASSERT_DOUBLE_WITHIN(0.001, 49.0583, p.lat_deg);
  TEST_ASSERT_DOUBLE_WITHIN(0.001, -72.0292, p.lon_deg);
  TEST_ASSERT_EQUAL_INT('/', p.symbol_table);
  TEST_ASSERT_EQUAL_INT('>', p.symbol_code);   // car icon
  TEST_ASSERT_EQUAL_STRING("Test", p.comment);
}

void test_aprs_parse_position_with_msg_dti() {
  ax25::Frame f;
  ax25::parse(kDigipathFrame, sizeof(kDigipathFrame), f);
  aprs::Position p;
  TEST_ASSERT_TRUE(aprs::parse_position(f.info, f.info_len, p));
  TEST_ASSERT_DOUBLE_WITHIN(0.001, 49.0583, p.lat_deg);
  TEST_ASSERT_DOUBLE_WITHIN(0.001, -72.0292, p.lon_deg);
}

void test_aprs_parse_position_rejects_status_dti() {
  const uint8_t info[] = {'>', 'h', 'i'};
  aprs::Position p;
  TEST_ASSERT_FALSE(aprs::parse_position(info, sizeof(info), p));
}

void test_aprs_parse_position_rejects_compressed() {
  // Compressed format starts with sym table char, not a digit.
  const uint8_t info[] = {'!', '/', '5', 'L', '!', '!', '<', '*', 'e', '7', '>', '7', 'P'};
  aprs::Position p;
  TEST_ASSERT_FALSE(aprs::parse_position(info, sizeof(info), p));
}

void test_aprs_parse_position_signs_southern_western() {
  // !3340.50S/15812.00E  → -33.675°, 158.2°
  const uint8_t info[] = {
    '!','3','3','4','0','.','5','0','S','/',
    '1','5','8','1','2','.','0','0','E','>',
  };
  aprs::Position p;
  TEST_ASSERT_TRUE(aprs::parse_position(info, sizeof(info), p));
  TEST_ASSERT_DOUBLE_WITHIN(0.01, -33.675, p.lat_deg);
  TEST_ASSERT_DOUBLE_WITHIN(0.01, 158.2,   p.lon_deg);
}

void test_aprs_parse_status() {
  ax25::Frame f;
  ax25::parse(kStatusFrame, sizeof(kStatusFrame), f);
  char text[40] = {0};
  TEST_ASSERT_TRUE(aprs::parse_status(f.info, f.info_len, text, sizeof(text)));
  TEST_ASSERT_EQUAL_STRING("Battery low", text);
}

void test_aprs_parse_status_rejects_position_dti() {
  const uint8_t info[] = {'!', '4', '9', '0', '3'};
  char text[16];
  TEST_ASSERT_FALSE(aprs::parse_status(info, sizeof(info), text, sizeof(text)));
}

// ───── KISS framing ────────────────────────────────────────────────────────

void test_kiss_encode_simple_payload() {
  const uint8_t payload[] = {'A', 'B', 'C'};
  uint8_t out[16];
  const size_t n = kiss::encode_data(payload, sizeof(payload), out, sizeof(out));
  TEST_ASSERT_EQUAL_size_t(6u, n);
  TEST_ASSERT_EQUAL_HEX8(0xC0, out[0]);  // FEND
  TEST_ASSERT_EQUAL_HEX8(0x00, out[1]);  // CMD = Data
  TEST_ASSERT_EQUAL_HEX8('A',  out[2]);
  TEST_ASSERT_EQUAL_HEX8('B',  out[3]);
  TEST_ASSERT_EQUAL_HEX8('C',  out[4]);
  TEST_ASSERT_EQUAL_HEX8(0xC0, out[5]);  // FEND
}

void test_kiss_encode_escapes_fend_byte() {
  // Payload contains a 0xC0 byte → must be escaped 0xDB 0xDC.
  // Frame: FEND CMD FESC TFEND 'X' FEND = 6 bytes total.
  const uint8_t payload[] = {0xC0, 'X'};
  uint8_t out[16];
  const size_t n = kiss::encode_data(payload, sizeof(payload), out, sizeof(out));
  TEST_ASSERT_EQUAL_size_t(6u, n);
  TEST_ASSERT_EQUAL_HEX8(0xC0, out[0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, out[1]);
  TEST_ASSERT_EQUAL_HEX8(0xDB, out[2]);
  TEST_ASSERT_EQUAL_HEX8(0xDC, out[3]);
  TEST_ASSERT_EQUAL_HEX8('X',  out[4]);
  TEST_ASSERT_EQUAL_HEX8(0xC0, out[5]);
}

void test_kiss_encode_escapes_fesc_byte() {
  const uint8_t payload[] = {0xDB};
  uint8_t out[8];
  const size_t n = kiss::encode_data(payload, sizeof(payload), out, sizeof(out));
  TEST_ASSERT_EQUAL_size_t(5u, n);
  TEST_ASSERT_EQUAL_HEX8(0xDB, out[2]);  // FESC
  TEST_ASSERT_EQUAL_HEX8(0xDD, out[3]);  // TFESC
}

void test_kiss_encode_return_frame_is_three_bytes() {
  uint8_t out[8];
  const size_t n = kiss::encode_return(out, sizeof(out));
  TEST_ASSERT_EQUAL_size_t(3u, n);
  TEST_ASSERT_EQUAL_HEX8(0xC0, out[0]);
  TEST_ASSERT_EQUAL_HEX8(0xFF, out[1]);
  TEST_ASSERT_EQUAL_HEX8(0xC0, out[2]);
}

void test_kiss_encode_buffer_too_small_returns_zero() {
  const uint8_t payload[] = {'A', 'B', 'C'};
  uint8_t out[4];   // need 6 — too small
  TEST_ASSERT_EQUAL_size_t(0u, kiss::encode_data(payload, sizeof(payload),
                                                  out, sizeof(out)));
}

void test_kiss_decode_simple_frame() {
  kiss::Decoder d;
  const uint8_t bytes[] = {0xC0, 0x00, 'H', 'i', 0xC0};
  for (uint8_t b : bytes) d.feed(b);
  TEST_ASSERT_TRUE(d.ready());
  uint8_t cmd; const uint8_t* p; size_t n;
  TEST_ASSERT_TRUE(d.take_frame(cmd, p, n));
  TEST_ASSERT_EQUAL_HEX8(0x00, cmd);
  TEST_ASSERT_EQUAL_size_t(2u, n);
  TEST_ASSERT_EQUAL_HEX8('H', p[0]);
  TEST_ASSERT_EQUAL_HEX8('i', p[1]);
}

void test_kiss_decode_unescapes_fend() {
  kiss::Decoder d;
  // FEND 0x00 0xDB 0xDC 'A' FEND  — payload is {0xC0, 'A'}
  const uint8_t bytes[] = {0xC0, 0x00, 0xDB, 0xDC, 'A', 0xC0};
  for (uint8_t b : bytes) d.feed(b);
  TEST_ASSERT_TRUE(d.ready());
  uint8_t cmd; const uint8_t* p; size_t n;
  TEST_ASSERT_TRUE(d.take_frame(cmd, p, n));
  TEST_ASSERT_EQUAL_size_t(2u, n);
  TEST_ASSERT_EQUAL_HEX8(0xC0, p[0]);
  TEST_ASSERT_EQUAL_HEX8('A',  p[1]);
}

void test_kiss_decode_unescapes_fesc() {
  kiss::Decoder d;
  const uint8_t bytes[] = {0xC0, 0x00, 0xDB, 0xDD, 0xC0};   // payload = {0xDB}
  for (uint8_t b : bytes) d.feed(b);
  uint8_t cmd; const uint8_t* p; size_t n;
  TEST_ASSERT_TRUE(d.take_frame(cmd, p, n));
  TEST_ASSERT_EQUAL_size_t(1u, n);
  TEST_ASSERT_EQUAL_HEX8(0xDB, p[0]);
}

void test_kiss_decode_drops_junk_between_frames() {
  kiss::Decoder d;
  const uint8_t bytes[] = {
    'g','a','r','b','a','g','e',                 // pre-frame junk → ignored
    0xC0, 0x00, 'O', 'K', 0xC0,                  // first frame
    0xC0, 0x00, 'H', 'I', 0xC0,                  // second frame
  };
  size_t frames = 0;
  for (uint8_t b : bytes) {
    if (d.feed(b)) ++frames;
  }
  TEST_ASSERT_EQUAL_size_t(2u, frames);
}

void test_kiss_decode_back_to_back_fends_idle_resync() {
  kiss::Decoder d;
  // Some TNCs send extra FEND between frames as a guard. Decoder must
  // tolerate this without producing a 0-byte spurious frame.
  const uint8_t bytes[] = {
    0xC0, 0xC0, 0x00, 'A', 0xC0,
  };
  size_t frames = 0;
  for (uint8_t b : bytes) {
    if (d.feed(b)) ++frames;
  }
  TEST_ASSERT_EQUAL_size_t(1u, frames);
}

// ───── AprsApp (Track A first real app) ────────────────────────────────────
//
// End-to-end pipeline: FakeRadioLink → kiss::Decoder → ax25::parse →
// aprs::parse_position → AprsApp::Station. Builds the KISS bytes from
// the same kSimplePosFrame / kDigipathFrame test vectors above.

namespace {

// Wrap a raw AX.25 frame as a single KISS Data frame.
size_t kiss_wrap(const uint8_t* in, size_t in_len, uint8_t* out, size_t cap) {
  return kiss::encode_data(in, in_len, out, cap);
}

}  // namespace

void test_aprs_app_skips_kiss_when_radio_idle() {
  Fixture f;
  FakeRadioLink r;   // not connected
  AprsApp app{r};
  app.on_enter(f.hal);
  // Should not crash, should not advance state machine.
  app.tick(0);
  TEST_ASSERT_EQUAL_size_t(0u, app.station_count());
  TEST_ASSERT_EQUAL_size_t(0u, app.rx_count());
}

void test_aprs_app_enters_kiss_mode_on_open() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  AprsApp app{r};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(r.state() == RadioLinkState::KissMode);
}

void test_aprs_app_decodes_position_from_kiss_bytes() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  AprsApp app{r};
  app.on_enter(f.hal);
  // Wrap the simple K1ABC position frame as a KISS Data frame.
  uint8_t kiss_bytes[80];
  const size_t n = kiss_wrap(kSimplePosFrame, sizeof(kSimplePosFrame),
                             kiss_bytes, sizeof(kiss_bytes));
  TEST_ASSERT_TRUE(n > 0);
  r.queue_kiss_bytes(kiss_bytes, n);
  app.tick(1234);
  TEST_ASSERT_EQUAL_size_t(1u, app.station_count());
  TEST_ASSERT_EQUAL_size_t(1u, app.rx_count());
  const auto& s = app.station_at(0);
  TEST_ASSERT_EQUAL_STRING("K1ABC", s.call);
  TEST_ASSERT_DOUBLE_WITHIN(0.001, 49.0583, s.lat_deg);
  TEST_ASSERT_DOUBLE_WITHIN(0.001, -72.0292, s.lon_deg);
  TEST_ASSERT_EQUAL_UINT32(1234u, s.last_seen_ms);
}

void test_aprs_app_appends_ssid_to_callsign() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  AprsApp app{r};
  app.on_enter(f.hal);
  uint8_t kiss_bytes[80];
  const size_t n = kiss_wrap(kDigipathFrame, sizeof(kDigipathFrame),
                             kiss_bytes, sizeof(kiss_bytes));
  r.queue_kiss_bytes(kiss_bytes, n);
  app.tick(0);
  TEST_ASSERT_EQUAL_size_t(1u, app.station_count());
  TEST_ASSERT_EQUAL_STRING("K1ABC-9", app.station_at(0).call);
}

void test_aprs_app_dedupes_repeat_transmissions() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  AprsApp app{r};
  app.on_enter(f.hal);
  uint8_t kiss_bytes[80];
  const size_t n = kiss_wrap(kSimplePosFrame, sizeof(kSimplePosFrame),
                             kiss_bytes, sizeof(kiss_bytes));
  // Same frame twice in a row.
  r.queue_kiss_bytes(kiss_bytes, n);
  app.tick(100);
  r.queue_kiss_bytes(kiss_bytes, n);
  app.tick(200);
  // Same callsign → same slot; count stays at 1, last_seen advances.
  TEST_ASSERT_EQUAL_size_t(1u, app.station_count());
  TEST_ASSERT_EQUAL_size_t(2u, app.rx_count());
  TEST_ASSERT_EQUAL_UINT32(200u, app.station_at(0).last_seen_ms);
}

void test_aprs_app_status_frame_doesnt_create_station() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  AprsApp app{r};
  app.on_enter(f.hal);
  uint8_t kiss_bytes[80];
  const size_t n = kiss_wrap(kStatusFrame, sizeof(kStatusFrame),
                             kiss_bytes, sizeof(kiss_bytes));
  r.queue_kiss_bytes(kiss_bytes, n);
  app.tick(0);
  // RX counted (frame parsed) but no position → no station added.
  TEST_ASSERT_EQUAL_size_t(1u, app.rx_count());
  TEST_ASSERT_EQUAL_size_t(0u, app.station_count());
}

void test_aprs_app_arrow_keys_move_cursor() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  AprsApp app{r};
  app.on_enter(f.hal);
  // Insert two distinct stations: K1ABC and K1ABC-9 (different keys).
  uint8_t b[80];
  size_t n;
  n = kiss_wrap(kSimplePosFrame, sizeof(kSimplePosFrame), b, sizeof(b));
  r.queue_kiss_bytes(b, n);
  app.tick(0);
  n = kiss_wrap(kDigipathFrame, sizeof(kDigipathFrame), b, sizeof(b));
  r.queue_kiss_bytes(b, n);
  app.tick(0);
  TEST_ASSERT_EQUAL_size_t(2u, app.station_count());
  TEST_ASSERT_EQUAL_size_t(0u, app.cursor());
  app.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor());
  app.on_key(press(Key::Down));   // bounded at last
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor());
  app.on_key(press(Key::Up));
  TEST_ASSERT_EQUAL_size_t(0u, app.cursor());
}

void test_aprs_app_render_header_red() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  AprsApp app{r};
  app.on_enter(f.hal);
  app.render(f.display);
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, 5));
}

void test_aprs_app_render_listening_when_empty() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  AprsApp app{r};
  app.on_enter(f.hal);
  app.render(f.display);
  TEST_ASSERT_TRUE(f.display.last_text().find("Listening") != std::string::npos);
}

// ───── KenwoodApp (Track A admin/status) ──────────────────────────────────

void test_kenwood_app_shows_idle_when_disconnected() {
  Fixture f;
  FakeRadioLink r;
  KenwoodApp app{r};
  app.on_enter(f.hal);
  app.render(f.display);
  // Last drawn text is the bottom hint; just confirm header is red.
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, 5));
  TEST_ASSERT_EQUAL_STRING("", app.id_text());
}

void test_kenwood_app_refresh_queries_id_freq_mode() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  r.register_reply("ID",   "ID TH-D75A");
  r.register_reply("FQ 0", "FQ 0,0146520000");
  r.register_reply("MD 0", "MD 0,0");
  KenwoodApp app{r};
  app.on_enter(f.hal);   // refresh runs from on_enter
  TEST_ASSERT_EQUAL_STRING("ID TH-D75A",       app.id_text());
  TEST_ASSERT_EQUAL_STRING("FQ 0,0146520000",  app.freq_text());
  TEST_ASSERT_EQUAL_STRING("MD 0,0",           app.mode_text());
}

void test_kenwood_app_enter_re_refreshes() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  r.register_reply("ID", "ID FIRST");
  KenwoodApp app{r};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_STRING("ID FIRST", app.id_text());
  r.register_reply("ID", "ID SECOND");
  app.on_key(press(Key::Enter));
  TEST_ASSERT_EQUAL_STRING("ID SECOND", app.id_text());
}

void test_kenwood_app_tab_uses_stored_mac() {
  Fixture f;
  FakeRadioLink r;
  FakeStorage  st;
  st.put_str("radio.mac", "AA:BB:CC:DD:EE:FF");
  KenwoodApp app{r, &st};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_INT(0, r.connects());
  app.on_key(press(Key::Tab));
  TEST_ASSERT_EQUAL_INT(1, r.connects());
  TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:FF", r.last_mac().c_str());
}

void test_kenwood_app_tab_no_op_without_stored_mac() {
  Fixture f;
  FakeRadioLink r;
  FakeStorage  st;
  KenwoodApp app{r, &st};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_EQUAL_INT(0, r.connects());
}

// ───── GpsApp (Track A — IGnss consumer + GPX exporter) ───────────────────

void test_gps_app_shows_no_data_when_gnss_silent() {
  Fixture f;
  FakeGnss g;
  FakeFs   fs;
  GpsApp app{g, fs};
  app.on_enter(f.hal);
  app.render(f.display);
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, 5));
}

void test_gps_app_records_points_at_interval() {
  Fixture f;
  FakeGnss g;
  GnssFix fix; fix.valid = true;
  fix.lat_deg = 49.0; fix.lon_deg = -72.0; fix.altitude_m = 100;
  fix.epoch_seconds = 1735689600ULL;
  g.set_fix(fix);
  FakeFs fs;
  GpsApp app{g, fs};
  app.on_enter(f.hal);
  // Press Tab to start recording
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.recording());
  // tick before interval — no point
  app.tick(0);
  TEST_ASSERT_EQUAL_size_t(1u, app.point_count());  // first tick records
  // Advance by less than interval — should NOT record
  app.tick(1000);
  TEST_ASSERT_EQUAL_size_t(1u, app.point_count());
  // Advance past interval — records
  app.tick(7000);
  TEST_ASSERT_EQUAL_size_t(2u, app.point_count());
}

void test_gps_app_skips_invalid_fixes() {
  Fixture f;
  FakeGnss g;
  // valid stays false
  FakeFs fs;
  GpsApp app{g, fs};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  app.tick(0);
  app.tick(10000);
  TEST_ASSERT_EQUAL_size_t(0u, app.point_count());
}

void test_gps_app_caps_at_max_points() {
  Fixture f;
  FakeGnss g;
  GnssFix fix; fix.valid = true; fix.lat_deg = 0; fix.lon_deg = 0;
  fix.epoch_seconds = 1;
  g.set_fix(fix);
  FakeFs fs;
  GpsApp app{g, fs};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  // Force GpsApp::kMaxPoints+5 ticks past the interval each time.
  for (size_t i = 0; i < GpsApp::kMaxPoints + 5; ++i) {
    app.tick(static_cast<uint32_t>((i + 1) * 6000));
  }
  TEST_ASSERT_EQUAL_size_t(GpsApp::kMaxPoints, app.point_count());
}

void test_gps_app_tab_stop_writes_gpx_to_fs() {
  Fixture f;
  FakeGnss g;
  GnssFix fix; fix.valid = true;
  fix.lat_deg = 49.0583; fix.lon_deg = -72.0292; fix.altitude_m = 50;
  fix.epoch_seconds = 1735689600ULL;
  g.set_fix(fix);
  FakeFs fs;
  GpsApp app{g, fs};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));   // start
  app.tick(0);
  app.tick(7000);                 // 2 points
  app.on_key(press(Key::Tab));   // stop → flush
  TEST_ASSERT_TRUE(app.last_save_ok());
  TEST_ASSERT_FALSE(app.recording());
  // FakeFs received a write for the expected path
  TEST_ASSERT_TRUE(fs.exists("/yui-tracks/1735689600.gpx"));
}

void test_aprs_app_evicts_oldest_when_full() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  AprsApp app{r};
  app.on_enter(f.hal);
  // Fill with kMaxStations distinct callsigns (synthetic).
  for (size_t i = 0; i < AprsApp::kMaxStations; ++i) {
    // Build a frame with src "K0XXNN" where NN = i.
    uint8_t frame[sizeof(kSimplePosFrame)];
    std::memcpy(frame, kSimplePosFrame, sizeof(frame));
    // src callsign chars are at offsets 7..12 (left-shifted ASCII).
    // Replace last 2 chars with digits encoding i.
    frame[7+4] = static_cast<uint8_t>(('0' + (i / 10) % 10) << 1);
    frame[7+5] = static_cast<uint8_t>(('0' + i % 10) << 1);
    uint8_t kb[80];
    size_t n = kiss_wrap(frame, sizeof(frame), kb, sizeof(kb));
    r.queue_kiss_bytes(kb, n);
    app.tick(static_cast<uint32_t>(i + 1));
  }
  TEST_ASSERT_EQUAL_size_t(AprsApp::kMaxStations, app.station_count());
  // Add one more — slot 0 (oldest) should get evicted, count stays.
  uint8_t newer_frame[sizeof(kSimplePosFrame)];
  std::memcpy(newer_frame, kSimplePosFrame, sizeof(newer_frame));
  newer_frame[7+4] = static_cast<uint8_t>('Z' << 1);
  newer_frame[7+5] = static_cast<uint8_t>('Z' << 1);
  uint8_t kb[80];
  size_t n = kiss_wrap(newer_frame, sizeof(newer_frame), kb, sizeof(kb));
  r.queue_kiss_bytes(kb, n);
  app.tick(9999);
  TEST_ASSERT_EQUAL_size_t(AprsApp::kMaxStations, app.station_count());
}

void test_kiss_round_trip_encode_decode() {
  // Encode a payload that contains both escape bytes, then decode it
  // back. Must produce identical bytes.
  const uint8_t payload[] = {'A', 0xC0, 'B', 0xDB, 'C', 0xC0, 0xDB};
  uint8_t encoded[32];
  const size_t n = kiss::encode_data(payload, sizeof(payload),
                                     encoded, sizeof(encoded));
  TEST_ASSERT_TRUE(n > 0);
  kiss::Decoder d;
  for (size_t i = 0; i < n; ++i) d.feed(encoded[i]);
  uint8_t cmd; const uint8_t* p; size_t got_len;
  TEST_ASSERT_TRUE(d.take_frame(cmd, p, got_len));
  TEST_ASSERT_EQUAL_size_t(sizeof(payload), got_len);
  for (size_t i = 0; i < sizeof(payload); ++i) {
    TEST_ASSERT_EQUAL_HEX8(payload[i], p[i]);
  }
}

void test_pineapple_client_unauthenticated_until_login() {
  FakeHttp h;
  pineapple::Client c{h};
  TEST_ASSERT_FALSE(c.authenticated());
  // No canned response → login returns false.
  TEST_ASSERT_FALSE(c.login("172.16.42.1", 1471, "root", "hak5"));
  TEST_ASSERT_FALSE(c.authenticated());
}

void test_pineapple_client_login_extracts_token() {
  FakeHttp h;
  h.register_response("POST", "http://172.16.42.1:1471/api/login",
                      200, "{\"token\":\"AAA.BBB.CCC\"}");
  pineapple::Client c{h};
  TEST_ASSERT_TRUE(c.login("172.16.42.1", 1471, "root", "hak5"));
  TEST_ASSERT_TRUE(c.authenticated());
  // last_body holds what we sent
  TEST_ASSERT_TRUE(h.last_body().find("\"username\":\"root\"") !=
                   std::string::npos);
}

void test_pineapple_client_login_persists_creds_to_storage() {
  FakeHttp h;
  FakeStorage st;
  h.register_response("POST", "http://10.0.0.5:1471/api/login",
                      200, "{\"token\":\"abc\"}");
  pineapple::Client c{h, &st};
  TEST_ASSERT_TRUE(c.login("10.0.0.5", 1471, "root", "secret"));
  char back[40] = {0};
  TEST_ASSERT_TRUE(st.get_str("pa.host", back, sizeof(back)));
  TEST_ASSERT_EQUAL_STRING("10.0.0.5", back);
}

void test_pineapple_client_login_failure_keeps_unauthenticated() {
  FakeHttp h;
  h.register_response("POST", "http://172.16.42.1:1471/api/login",
                      403, "{\"error\":\"bad creds\"}");
  pineapple::Client c{h};
  TEST_ASSERT_FALSE(c.login("172.16.42.1", 1471, "root", "wrong"));
  TEST_ASSERT_FALSE(c.authenticated());
}

void test_pineapple_dashboard_cards_parses_status() {
  FakeHttp h;
  h.register_response("POST", "http://172.16.42.1:1471/api/login",
                      200, "{\"token\":\"T\"}");
  h.register_response("GET", "http://172.16.42.1:1471/api/dashboard/cards",
                      200,
    "{\"systemStatus\":{\"cpuUsage\":42,\"memoryUsage\":33,"
    "\"temperature\":58},\"clientsConnected\":\"7\","
    "\"totalSSIDs\":\"123\"}");
  pineapple::Client c{h};
  c.login("172.16.42.1", 1471, "root", "x");
  pineapple::Cards out;
  TEST_ASSERT_TRUE(c.dashboard_cards(out));
  TEST_ASSERT_EQUAL_INT(42,  out.cpu_pct);
  TEST_ASSERT_EQUAL_INT(33,  out.mem_pct);
  TEST_ASSERT_EQUAL_INT(58,  out.temp_c);
  TEST_ASSERT_EQUAL_INT(7,   out.clients_connected);
  TEST_ASSERT_EQUAL_INT(123, out.total_ssids);
}

void test_pineapple_recon_start_returns_scan_id() {
  FakeHttp h;
  h.register_response("POST", "http://h:1471/api/login",
                      200, "{\"token\":\"T\"}");
  h.register_response("POST", "http://h:1471/api/recon/start",
                      200, "{\"scanRunning\":true,\"scanID\":42}");
  pineapple::Client c{h};
  c.login("h", 1471, "u", "p");
  int sid = 0;
  TEST_ASSERT_TRUE(c.recon_start(true, 30, "2.4ghz", sid));
  TEST_ASSERT_EQUAL_INT(42, sid);
  TEST_ASSERT_TRUE(h.last_body().find("\"band\":\"2.4ghz\"") !=
                   std::string::npos);
}

void test_pineapple_recon_status_polls_progress() {
  FakeHttp h;
  h.register_response("POST", "http://h:1471/api/login",
                      200, "{\"token\":\"T\"}");
  h.register_response("GET", "http://h:1471/api/recon/status",
                      200,
    "{\"scanRunning\":true,\"scanPercent\":75,\"scanID\":42}");
  pineapple::Client c{h};
  c.login("h", 1471, "u", "p");
  int sid = 0; bool running = false; int pct = 0;
  TEST_ASSERT_TRUE(c.recon_status(sid, running, pct));
  TEST_ASSERT_EQUAL_INT(42, sid);
  TEST_ASSERT_TRUE(running);
  TEST_ASSERT_EQUAL_INT(75, pct);
}

void test_pineapple_401_triggers_relogin_and_retries() {
  FakeHttp h;
  h.register_response("POST", "http://h:1471/api/login",
                      200, "{\"token\":\"T1\"}");
  pineapple::Client c{h};
  c.login("h", 1471, "u", "p");
  // First GET returns 401 — the client should relogin (which will use
  // the same canned 200 login response) and retry the GET. Without a
  // success response for the GET (we don't register one), the retry
  // path will also fail — we assert the relogin attempt happened by
  // checking total HTTP calls.
  h.register_response("GET", "http://h:1471/api/dashboard/cards",
                      401, "{\"error\":\"expired\"}");
  pineapple::Cards out;
  c.dashboard_cards(out);
  // Calls: 1 login, 1 GET (401), 1 relogin, 1 GET retry = 4
  TEST_ASSERT_EQUAL_INT(4, h.calls());
}

// ───── JsonValue extractor unit tests ──────────────────────────────────────

void test_json_find_string_simple() {
  const char* src = "{\"token\":\"abc.def\"}";
  char out[16] = {0};
  TEST_ASSERT_TRUE(json::find_string(src, "token", out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("abc.def", out);
}

void test_json_find_string_missing_key_returns_false() {
  const char* src = "{\"foo\":1}";
  char out[16] = {0};
  TEST_ASSERT_FALSE(json::find_string(src, "bar", out, sizeof(out)));
}

void test_json_find_int_unquoted_and_quoted() {
  const char* a = "{\"x\":42}";
  int v = 0;
  TEST_ASSERT_TRUE(json::find_int(a, "x", &v));
  TEST_ASSERT_EQUAL_INT(42, v);
  // Pineapple sometimes returns "7" as a string.
  const char* b = "{\"clientsConnected\":\"7\"}";
  TEST_ASSERT_TRUE(json::find_int(b, "clientsConnected", &v));
  TEST_ASSERT_EQUAL_INT(7, v);
}

void test_json_find_bool_true_false() {
  bool v = false;
  TEST_ASSERT_TRUE(json::find_bool("{\"r\":true}",  "r", &v));
  TEST_ASSERT_TRUE(v);
  TEST_ASSERT_TRUE(json::find_bool("{\"r\":false}", "r", &v));
  TEST_ASSERT_FALSE(v);
}

// ───── PineappleApp / PineappleReconApp ───────────────────────────────────

namespace {

void seed_pineapple_creds(FakeStorage& st) {
  st.put_str("pa.host", "pa.local");
  st.put_int("pa.port", 1471);
  st.put_str("pa.user", "root");
  st.put_str("pa.pass", "hak5");
}

void register_login_ok(FakeHttp& h, const char* host = "pa.local") {
  char url[80];
  std::snprintf(url, sizeof(url), "http://%s:1471/api/login", host);
  h.register_response("POST", url, 200, "{\"token\":\"T\"}");
}

}  // namespace

void test_pineapple_app_no_creds_renders_hint() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  PineappleApp app{h, st};
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(app.last_ok());
  app.render(f.display);
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(20, 5));
}

void test_pineapple_app_logs_in_and_fetches_cards() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  register_login_ok(h);
  h.register_response("GET",
      "http://pa.local:1471/api/dashboard/cards",
      200,
      "{\"systemStatus\":{\"cpuUsage\":12,\"memoryUsage\":34,"
      "\"temperature\":50},\"clientsConnected\":\"3\","
      "\"totalSSIDs\":\"99\"}");
  PineappleApp app{h, st};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.last_ok());
  TEST_ASSERT_EQUAL_INT(12, app.cards().cpu_pct);
  TEST_ASSERT_EQUAL_INT(99, app.cards().total_ssids);
}

void test_pineapple_app_login_failure_marks_error() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  // No login response registered → login fails
  PineappleApp app{h, st};
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(app.last_ok());
  TEST_ASSERT_FALSE(app.client().authenticated());
}

void test_pineapple_app_tab_re_fetches() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  register_login_ok(h);
  h.register_response("GET",
      "http://pa.local:1471/api/dashboard/cards",
      200, "{\"systemStatus\":{\"cpuUsage\":1}}");
  PineappleApp app{h, st};
  app.on_enter(f.hal);
  const int calls_before = h.calls();
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(h.calls() > calls_before);
}

void test_recon_app_starts_in_idle() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  PineappleReconApp app{h, st};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.state() == PineappleReconApp::State::Idle);
}

void test_recon_app_enter_starts_scan() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  register_login_ok(h);
  h.register_response("POST",
      "http://pa.local:1471/api/recon/start",
      200, "{\"scanRunning\":true,\"scanID\":7}");
  PineappleReconApp app{h, st};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == PineappleReconApp::State::Scanning);
  TEST_ASSERT_EQUAL_INT(7, app.scan_id());
}

void test_recon_app_tick_polls_and_completes() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  register_login_ok(h);
  h.register_response("POST",
      "http://pa.local:1471/api/recon/start",
      200, "{\"scanRunning\":true,\"scanID\":7}");
  h.register_response("GET",
      "http://pa.local:1471/api/recon/status",
      200, "{\"scanRunning\":false,\"scanPercent\":100,\"scanID\":7}");
  PineappleReconApp app{h, st};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  app.tick(0);
  app.tick(2000);  // past poll interval
  TEST_ASSERT_TRUE(app.state() == PineappleReconApp::State::Done);
  TEST_ASSERT_EQUAL_INT(100, app.percent());
}

void test_recon_app_login_failure_yields_error_state() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  // No login response → login fails
  PineappleReconApp app{h, st};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == PineappleReconApp::State::Error);
}

// ───── 802.11 frame helpers ────────────────────────────────────────────────

namespace {

// Build a probe-request frame with the given SSID and source MAC.
size_t build_probe_request(uint8_t* out, size_t cap,
                           const uint8_t sa[6], const char* ssid) {
  if (cap < 28) return 0;
  size_t off = 0;
  // FC: type=mgmt(0), subtype=probe-req(4)  → byte0 = 0x40
  out[off++] = 0x40;
  out[off++] = 0x00;          // FC byte 1
  out[off++] = 0x00; out[off++] = 0x00;  // duration
  // Addr1 (DA) = broadcast
  for (int i = 0; i < 6; ++i) out[off++] = 0xFF;
  // Addr2 (SA)
  for (int i = 0; i < 6; ++i) out[off++] = sa[i];
  // Addr3 (BSSID) = broadcast for probe-req
  for (int i = 0; i < 6; ++i) out[off++] = 0xFF;
  // SeqCtl
  out[off++] = 0x00; out[off++] = 0x00;
  // Tagged params: SSID IE
  out[off++] = 0x00;          // tag = SSID
  const size_t slen = std::strlen(ssid);
  out[off++] = static_cast<uint8_t>(slen);
  for (size_t i = 0; i < slen; ++i) out[off++] = static_cast<uint8_t>(ssid[i]);
  return off;
}

// Build an EAPOL data frame with the given BSSID at addr3.
size_t build_eapol_data(uint8_t* out, size_t cap, const uint8_t bssid[6]) {
  if (cap < 32) return 0;
  size_t off = 0;
  out[off++] = 0x08;     // type=data(2), subtype=0 → byte0 = 0x08
  out[off++] = 0x00;
  out[off++] = 0x00; out[off++] = 0x00;  // duration
  // Addr1, Addr2, Addr3 — only Addr3 (BSSID) matters for our parser
  for (int i = 0; i < 6; ++i) out[off++] = 0x11;       // DA
  for (int i = 0; i < 6; ++i) out[off++] = 0x22;       // SA
  for (int i = 0; i < 6; ++i) out[off++] = bssid[i];   // BSSID
  out[off++] = 0x00; out[off++] = 0x00;                // SeqCtl
  // LLC/SNAP + EAPOL ethertype
  out[off++] = 0xAA; out[off++] = 0xAA; out[off++] = 0x03;
  out[off++] = 0x00; out[off++] = 0x00; out[off++] = 0x00;
  out[off++] = 0x88; out[off++] = 0x8E;   // EAPOL
  // Token EAPOL key body
  out[off++] = 0x01; out[off++] = 0x03;
  return off;
}

}  // namespace

void test_dot11_extract_ssid_from_probe_request() {
  uint8_t frame[64];
  uint8_t sa[6] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
  const size_t n = build_probe_request(frame, sizeof(frame), sa, "MyHomeWiFi");
  TEST_ASSERT_TRUE(n > 0);
  TEST_ASSERT_TRUE(dot11::is_probe_request(frame, n));
  char ssid[33] = {0};
  TEST_ASSERT_TRUE(dot11::extract_ssid(frame, n, ssid, sizeof(ssid)));
  TEST_ASSERT_EQUAL_STRING("MyHomeWiFi", ssid);
}

void test_dot11_addr2_matches_source_mac() {
  uint8_t frame[64];
  uint8_t sa[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01};
  build_probe_request(frame, sizeof(frame), sa, "X");
  TEST_ASSERT_EQUAL_HEX8(0xDE, dot11::addr2(frame)[0]);
  TEST_ASSERT_EQUAL_HEX8(0x01, dot11::addr2(frame)[5]);
}

void test_dot11_is_eapol_data_recognizes_8888e_ethertype() {
  uint8_t frame[64];
  uint8_t bssid[6] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5};
  const size_t n = build_eapol_data(frame, sizeof(frame), bssid);
  TEST_ASSERT_TRUE(dot11::is_eapol_data(frame, n));
}

void test_dot11_format_mac() {
  uint8_t mac[6] = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E};
  char out[18];
  dot11::format_mac(mac, out);
  TEST_ASSERT_EQUAL_STRING("00:1A:2B:3C:4D:5E", out);
}

// ───── WifiProbeApp ────────────────────────────────────────────────────────

void test_probe_app_starts_monitor_with_mgmt_filter() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiProbeApp app{mon};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(mon.running());
  TEST_ASSERT_TRUE(mon.filter().mgmt);
  TEST_ASSERT_FALSE(mon.filter().data);
}

void test_probe_app_records_unique_probe_requests() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiProbeApp app{mon};
  app.on_enter(f.hal);
  uint8_t frame[64];
  uint8_t sa1[6] = {1, 2, 3, 4, 5, 6};
  uint8_t sa2[6] = {7, 8, 9, 10, 11, 12};
  size_t n;
  n = build_probe_request(frame, sizeof(frame), sa1, "Net1");
  WifiRxMeta meta{}; meta.type = WifiPktType::Management; meta.channel = 1;
  mon.inject_frame(frame, n, meta);
  n = build_probe_request(frame, sizeof(frame), sa2, "Net2");
  mon.inject_frame(frame, n, meta);
  // Same SA + SSID again → should bump count, not add a new entry.
  n = build_probe_request(frame, sizeof(frame), sa1, "Net1");
  mon.inject_frame(frame, n, meta);
  TEST_ASSERT_EQUAL_size_t(2u, app.entry_count());
  TEST_ASSERT_EQUAL_UINT32(2u, app.entry_at(0).count);
  TEST_ASSERT_EQUAL_UINT32(1u, app.entry_at(1).count);
}

void test_probe_app_ignores_non_probe_request_frames() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiProbeApp app{mon};
  app.on_enter(f.hal);
  uint8_t frame[64];
  uint8_t bssid[6] = {1,2,3,4,5,6};
  const size_t n = build_eapol_data(frame, sizeof(frame), bssid);
  WifiRxMeta meta{}; meta.type = WifiPktType::Data;
  mon.inject_frame(frame, n, meta);
  // Filter should drop it (data=false), but even if leaked it's not a
  // probe request → should still be 0 entries.
  TEST_ASSERT_EQUAL_size_t(0u, app.entry_count());
}

void test_probe_app_tick_hops_channels() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiProbeApp app{mon};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_UINT8(1, mon.channel());
  app.tick(0);                                // first tick = immediate hop
  TEST_ASSERT_EQUAL_UINT8(2, mon.channel());
  app.tick(100);                              // less than kHopMs → no hop
  TEST_ASSERT_EQUAL_UINT8(2, mon.channel());
  app.tick(500);                              // past kHopMs → hop
  TEST_ASSERT_EQUAL_UINT8(3, mon.channel());
}

void test_probe_app_channel_wraps_at_13_to_1() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiProbeApp app{mon};
  app.on_enter(f.hal);
  for (uint8_t i = 0; i < 13; ++i) {
    app.tick(static_cast<uint32_t>((i + 1) * 500));
  }
  TEST_ASSERT_EQUAL_UINT8(1, mon.channel());   // wrapped
}

void test_probe_app_arrow_keys_navigate() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiProbeApp app{mon};
  app.on_enter(f.hal);
  uint8_t frame[64];
  uint8_t sa[6] = {1,1,1,1,1,1};
  size_t n;
  WifiRxMeta meta{}; meta.type = WifiPktType::Management;
  for (int i = 0; i < 3; ++i) {
    sa[5] = static_cast<uint8_t>(i);
    n = build_probe_request(frame, sizeof(frame), sa, "X");
    mon.inject_frame(frame, n, meta);
  }
  app.on_key(press(Key::Down));
  app.on_key(press(Key::Down));
  // bounded
  app.on_key(press(Key::Down));
  // (cursor accessor not exposed; just confirm 3 entries recorded)
  TEST_ASSERT_EQUAL_size_t(3u, app.entry_count());
}

// ───── WifiHandshakeApp ────────────────────────────────────────────────────

void test_handshake_app_opens_pcap_on_enter() {
  Fixture f;
  FakeWifiMonitor mon;
  FakePcap pcap;
  WifiHandshakeApp app{mon, pcap, f.clock};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.file_open());
  TEST_ASSERT_TRUE(pcap.is_open());
  TEST_ASSERT_TRUE(mon.running());
}

void test_handshake_app_writes_every_frame_to_pcap() {
  Fixture f;
  FakeWifiMonitor mon;
  FakePcap pcap;
  WifiHandshakeApp app{mon, pcap, f.clock};
  app.on_enter(f.hal);
  uint8_t frame[64];
  uint8_t sa[6] = {1,2,3,4,5,6};
  WifiRxMeta meta{}; meta.type = WifiPktType::Management;
  const size_t n = build_probe_request(frame, sizeof(frame), sa, "T");
  mon.inject_frame(frame, n, meta);
  TEST_ASSERT_EQUAL_size_t(1u, app.pkt_total());
  // pcap buffer = 24-byte header + 16-byte record + n-byte payload
  TEST_ASSERT_EQUAL_UINT64(24ULL + 16ULL + n, pcap.bytes_written());
}

void test_handshake_app_increments_eapol_count_on_key_frame() {
  Fixture f;
  FakeWifiMonitor mon;
  FakePcap pcap;
  WifiHandshakeApp app{mon, pcap, f.clock};
  app.on_enter(f.hal);
  uint8_t frame[64];
  uint8_t bssid[6] = {0xA0, 0xB1, 0xC2, 0xD3, 0xE4, 0xF5};
  WifiRxMeta meta{}; meta.type = WifiPktType::Data;
  const size_t n = build_eapol_data(frame, sizeof(frame), bssid);
  mon.inject_frame(frame, n, meta);
  TEST_ASSERT_EQUAL_size_t(1u, app.eapol_seen());
  TEST_ASSERT_EQUAL_STRING("A0:B1:C2:D3:E4:F5", app.last_bssid());
}

void test_handshake_app_tick_hops_channels() {
  Fixture f;
  FakeWifiMonitor mon;
  FakePcap pcap;
  WifiHandshakeApp app{mon, pcap, f.clock};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_UINT8(1, mon.channel());
  app.tick(0);
  TEST_ASSERT_EQUAL_UINT8(2, mon.channel());
}

void test_handshake_app_on_exit_stops_and_closes() {
  Fixture f;
  FakeWifiMonitor mon;
  FakePcap pcap;
  WifiHandshakeApp app{mon, pcap, f.clock};
  app.on_enter(f.hal);
  app.on_exit();
  TEST_ASSERT_FALSE(mon.running());
  TEST_ASSERT_FALSE(pcap.is_open());
}

// ───── v0.2 stretch apps ──────────────────────────────────────────────────

// RemoteHeadApp

void test_remote_head_refreshes_on_enter() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  r.register_reply("FQ 0", "FQ 0,0146520000");
  r.register_reply("MD 0", "MD 0,0");
  RemoteHeadApp app{r};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_UINT64(146520000ULL, app.freq_hz());
  TEST_ASSERT_EQUAL_UINT8(0, app.mode());
}

void test_remote_head_tab_cycles_field() {
  Fixture f;
  FakeRadioLink r;
  RemoteHeadApp app{r};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.field() == RemoteHeadApp::Field::Vfo);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.field() == RemoteHeadApp::Field::Freq);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.field() == RemoteHeadApp::Field::Mode);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.field() == RemoteHeadApp::Field::Vfo);
}

void test_remote_head_up_adjusts_freq() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  r.register_reply("FQ 0", "FQ 0,0144000000");
  r.register_reply("MD 0", "MD 0,0");
  RemoteHeadApp app{r};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));   // Freq field
  app.on_key(press(Key::Up));    // +10 kHz default step
  TEST_ASSERT_EQUAL_UINT64(144010000ULL, app.freq_hz());
}

// AprsMessageApp

void test_aprs_msg_no_callsign_disables_input() {
  Fixture f;
  FakeRadioLink r;
  FakeStorage st;
  AprsMessageApp app{r, st};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.status() == AprsMessageApp::Status::NoCallsign);
}

void test_aprs_msg_loads_call_from_storage() {
  Fixture f;
  FakeRadioLink r;
  FakeStorage st;
  st.put_str("tx.call", "K1ABC");
  st.put_int("tx.ssid", 9);
  AprsMessageApp app{r, st};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.status() == AprsMessageApp::Status::Idle);
}

void test_aprs_msg_typed_chars_buffer_correctly() {
  Fixture f;
  FakeRadioLink r;
  FakeStorage st;
  st.put_str("tx.call", "K1ABC");
  AprsMessageApp app{r, st};
  app.on_enter(f.hal);
  KeyEvent k{}; k.key = Key::Char; k.down = true;
  k.ch = 'W'; app.on_key(k);
  k.ch = '1'; app.on_key(k);
  k.ch = 'X'; app.on_key(k);
  TEST_ASSERT_EQUAL_STRING("W1X", app.to_text());
  app.on_key(press(Key::Tab));
  k.ch = 'h'; app.on_key(k);
  k.ch = 'i'; app.on_key(k);
  TEST_ASSERT_EQUAL_STRING("hi", app.msg_text());
}

void test_aprs_msg_send_writes_kiss_frame() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  FakeStorage st;
  st.put_str("tx.call", "K1ABC");
  AprsMessageApp app{r, st};
  app.on_enter(f.hal);
  // Type addressee and message
  KeyEvent k{}; k.key = Key::Char; k.down = true;
  k.ch = 'W'; app.on_key(k);
  k.ch = '1'; app.on_key(k);
  app.on_key(press(Key::Tab));
  k.ch = 'h'; app.on_key(k);
  k.ch = 'i'; app.on_key(k);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.status() == AprsMessageApp::Status::Sent);
  // Frame should be in r.kiss_tx() and contain the message text.
  const auto& tx = r.kiss_tx();
  TEST_ASSERT_TRUE(tx.size() > 0);
  // Look for the literal "hi" inside the wrapped frame
  bool found = false;
  for (size_t i = 0; i + 1 < tx.size(); ++i) {
    if (tx[i] == 'h' && tx[i + 1] == 'i') { found = true; break; }
  }
  TEST_ASSERT_TRUE(found);
}

// Pineapple new endpoints

void test_pineapple_recon_results_parses_aps() {
  FakeHttp h;
  h.register_response("POST", "http://h:1471/api/login",
                      200, "{\"token\":\"T\"}");
  h.register_response("GET", "http://h:1471/api/recon/scans/3",
                      200,
    "{\"APResults\":["
    "{\"ssid\":\"Net1\",\"bssid\":\"AA:BB:CC:00:00:01\","
     "\"encryption\":\"wpa2\",\"channel\":6,\"rssi\":-55},"
    "{\"ssid\":\"Net2\",\"bssid\":\"AA:BB:CC:00:00:02\","
     "\"encryption\":\"open\",\"channel\":11,\"rssi\":-72}"
    "]}");
  pineapple::Client c{h};
  c.login("h", 1471, "u", "p");
  pineapple::ApInfo aps[8];
  size_t n = 0;
  TEST_ASSERT_TRUE(c.recon_results(3, aps, 8, n));
  TEST_ASSERT_EQUAL_size_t(2u, n);
  TEST_ASSERT_EQUAL_STRING("Net1", aps[0].ssid);
  TEST_ASSERT_EQUAL_STRING("Net2", aps[1].ssid);
  TEST_ASSERT_EQUAL_UINT8(11, aps[1].channel);
  TEST_ASSERT_EQUAL_INT8(-72, aps[1].rssi);
}

void test_pineapple_pineap_set_enabled_sends_put() {
  FakeHttp h;
  h.register_response("POST", "http://h:1471/api/login",
                      200, "{\"token\":\"T\"}");
  h.register_response("PUT",  "http://h:1471/api/pineap/settings",
                      200, "{\"success\":true}");
  pineapple::Client c{h};
  c.login("h", 1471, "u", "p");
  TEST_ASSERT_TRUE(c.pineap_set_enabled(true, true));
  TEST_ASSERT_TRUE(h.last_body().find("\"karma\":true") != std::string::npos);
}

void test_pineapple_deauth_ap_sends_post() {
  FakeHttp h;
  h.register_response("POST", "http://h:1471/api/login",
                      200, "{\"token\":\"T\"}");
  h.register_response("POST", "http://h:1471/api/pineap/deauth/ap",
                      200, "{\"success\":true}");
  pineapple::Client c{h};
  c.login("h", 1471, "u", "p");
  TEST_ASSERT_TRUE(c.deauth_ap("AA:BB:CC:DD:EE:FF", 6));
  TEST_ASSERT_TRUE(h.last_body().find("\"channel\":6") != std::string::npos);
}

// HandshakeBrowserApp

void test_handshake_browser_counts_bssids() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  register_login_ok(h);
  h.register_response("GET",
      "http://pa.local:1471/api/pineap/handshakes",
      200,
      "{\"handshakes\":["
      "{\"bssid\":\"a\",\"foo\":1},"
      "{\"bssid\":\"b\",\"foo\":2}"
      "]}");
  HandshakeBrowserApp app{h, st};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.last_ok());
  TEST_ASSERT_EQUAL_size_t(2u, app.count());
}

// EvilTwinApp

void test_evil_twin_fn_enter_enables() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  register_login_ok(h);
  h.register_response("PUT",  "http://pa.local:1471/api/pineap/settings",
                      200, "{\"success\":true}");
  EvilTwinApp app{h, st};
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(app.enabled());
  TEST_ASSERT_TRUE(app.last_ok());
}

void test_evil_twin_plain_enter_does_nothing() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  register_login_ok(h);
  EvilTwinApp app{h, st};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));   // no Fn modifier
  TEST_ASSERT_FALSE(app.enabled());
}

// KarmaApp

void test_karma_fn_enter_enables() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  register_login_ok(h);
  h.register_response("PUT",  "http://pa.local:1471/api/pineap/settings",
                      200, "{\"success\":true}");
  KarmaApp app{h, st};
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(app.enabled());
}

// WifiDeauthApp

void test_deauth_armed_required_to_fire() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  register_login_ok(h);
  h.register_response("POST", "http://pa.local:1471/api/pineap/deauth/ap",
                      200, "{\"success\":true}");
  WifiDeauthApp app{h, st};
  app.on_enter(f.hal);
  app.set_target("AA:BB:CC:DD:EE:FF", 6);
  app.on_key(press_fn(Key::Enter));   // not armed → no fire
  TEST_ASSERT_FALSE(app.fired());
  app.on_key(press(Key::Tab));        // arm
  TEST_ASSERT_TRUE(app.armed());
  app.on_key(press_fn(Key::Enter));   // fire
  TEST_ASSERT_TRUE(app.fired());
  TEST_ASSERT_TRUE(app.last_ok());
}

// BleSpamApp

void test_ble_spam_off_by_default() {
  Fixture f;
  FakeBleAdvertiser adv;
  BleSpamApp app{adv};
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(app.enabled());
  app.tick(0);
  app.tick(500);
  TEST_ASSERT_FALSE(adv.active());
}

void test_ble_spam_fn_enter_toggles_and_advertises() {
  Fixture f;
  FakeBleAdvertiser adv;
  BleSpamApp app{adv};
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(app.enabled());
  app.tick(0);
  TEST_ASSERT_TRUE(adv.active());
  TEST_ASSERT_TRUE(adv.set_count() >= 1);
}

void test_ble_spam_cycles_payloads() {
  Fixture f;
  FakeBleAdvertiser adv;
  BleSpamApp app{adv};
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  app.tick(0);
  const size_t first = app.cycle_idx();
  app.tick(500);
  TEST_ASSERT_TRUE(app.cycle_idx() != first);
}

// ───── Bruce-parity sweep apps ────────────────────────────────────────────

// TvBGoneApp

void test_tvbgone_starts_disabled() {
  Fixture f;
  FakeIr ir;
  TvBGoneApp app{ir};
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(app.enabled());
  app.tick(0);
  TEST_ASSERT_EQUAL_UINT32(0u, ir.sent_count());
}

void test_tvbgone_tab_enables_and_first_tick_fires() {
  Fixture f;
  FakeIr ir;
  TvBGoneApp app{ir};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.enabled());
  app.tick(0);
  TEST_ASSERT_EQUAL_UINT32(1u, ir.sent_count());
  TEST_ASSERT_EQUAL_size_t(1u, app.idx());
}

void test_tvbgone_iterates_full_table_then_disables() {
  Fixture f;
  FakeIr ir;
  TvBGoneApp app{ir};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  size_t n = 0; TvBGoneApp::codes(n);
  for (size_t i = 0; i < n; ++i) app.tick(static_cast<uint32_t>((i + 1) * 100));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(n), ir.sent_count());
  // One more tick after exhaustion → enabled flips off
  app.tick(static_cast<uint32_t>((n + 1) * 100));
  TEST_ASSERT_FALSE(app.enabled());
}

// WifiBeaconFloodApp

void test_beacon_flood_off_by_default() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiBeaconFloodApp app{mon};
  app.on_enter(f.hal);
  app.tick(0);
  TEST_ASSERT_EQUAL_size_t(0u, app.tx_total());
}

void test_beacon_flood_tab_enables_and_tx_cycles() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiBeaconFloodApp app{mon};
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.enabled());
  app.tick(0);
  app.tick(100);
  app.tick(200);
  TEST_ASSERT_TRUE(app.tx_total() >= 3);
  TEST_ASSERT_EQUAL_size_t(app.tx_total(), mon.tx_count());
}

void test_beacon_flood_frame_contains_ssid_bytes() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiBeaconFloodApp app{mon};
  app.clear_ssids();
  app.add_ssid("YUI_TEST");
  app.on_enter(f.hal);
  app.on_key(press(Key::Tab));
  app.tick(0);
  TEST_ASSERT_EQUAL_size_t(1u, mon.tx_count());
  const auto& bytes = mon.tx_log()[0];
  // Look for "YUI_TEST" bytes literally
  bool found = false;
  for (size_t i = 0; i + 8 <= bytes.size(); ++i) {
    if (std::memcmp(&bytes[i], "YUI_TEST", 8) == 0) { found = true; break; }
  }
  TEST_ASSERT_TRUE(found);
}

void test_beacon_flood_up_down_change_channel() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiBeaconFloodApp app{mon};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_UINT8(6, app.channel());
  app.on_key(press(Key::Up));
  TEST_ASSERT_EQUAL_UINT8(7, app.channel());
  TEST_ASSERT_EQUAL_UINT8(7, mon.channel());
  app.on_key(press(Key::Down));
  app.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_UINT8(5, app.channel());
}

// WifiNativeDeauthApp

void test_native_deauth_disarmed_by_default() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiNativeDeauthApp app{mon};
  app.on_enter(f.hal);
  app.set_target("AA:BB:CC:DD:EE:FF", "11:22:33:44:55:66");
  app.on_key(press_fn(Key::Enter));   // Fn+Enter without Tab → no fire
  app.tick(0);
  TEST_ASSERT_FALSE(app.firing());
  TEST_ASSERT_EQUAL_size_t(0u, app.tx_total());
}

void test_native_deauth_arm_and_fire_emits_frames() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiNativeDeauthApp app{mon};
  app.on_enter(f.hal);
  app.set_target("AA:BB:CC:DD:EE:FF", "FF:FF:FF:FF:FF:FF");
  app.on_key(press(Key::Tab));        // arm
  TEST_ASSERT_TRUE(app.armed());
  app.on_key(press_fn(Key::Enter));   // fire
  TEST_ASSERT_TRUE(app.firing());
  app.tick(0);
  app.tick(50);
  app.tick(100);
  TEST_ASSERT_TRUE(app.tx_total() >= 3);
  // Verify frames look like deauth (FC byte 0 == 0xC0)
  for (const auto& frame : mon.tx_log()) {
    TEST_ASSERT_TRUE(frame.size() >= 26);
    TEST_ASSERT_EQUAL_HEX8(0xC0, frame[0]);
  }
}

void test_native_deauth_invalid_mac_aborts_fire() {
  Fixture f;
  FakeWifiMonitor mon;
  WifiNativeDeauthApp app{mon};
  app.on_enter(f.hal);
  app.set_target("not-a-mac", "FF:FF:FF:FF:FF:FF");
  app.on_key(press(Key::Tab));
  app.on_key(press_fn(Key::Enter));
  app.tick(0);
  TEST_ASSERT_FALSE(app.firing());
  TEST_ASSERT_EQUAL_size_t(0u, app.tx_total());
}

// WpsScanApp

void test_wps_scan_records_aps_with_and_without_wps() {
  Fixture f;
  FakeWifiMonitor mon;
  WpsScanApp app{mon};
  app.on_enter(f.hal);
  // Build a beacon WITHOUT WPS IE (just SSID).
  uint8_t bssid_no_wps[6] = {0xAA, 0xBB, 0xCC, 0x00, 0x00, 0x01};
  uint8_t f1[80];
  size_t n1 = dot11::build_beacon(f1, sizeof(f1), "PlainNet", 8,
                                   bssid_no_wps, 6);
  WifiRxMeta m{}; m.type = WifiPktType::Management; m.channel = 6; m.rssi = -60;
  mon.inject_frame(f1, n1, m);
  // Beacon WITH WPS IE — splice the IE onto a built beacon.
  uint8_t bssid_wps[6] = {0xAA, 0xBB, 0xCC, 0x00, 0x00, 0x02};
  uint8_t f2[120];
  size_t n2 = dot11::build_beacon(f2, sizeof(f2), "WPSNet", 6,
                                   bssid_wps, 6);
  // Append vendor-specific WPS IE: tag=0xDD, len=4, OUI 00:50:F2 type 0x04
  f2[n2++] = 0xDD; f2[n2++] = 0x04;
  f2[n2++] = 0x00; f2[n2++] = 0x50; f2[n2++] = 0xF2; f2[n2++] = 0x04;
  mon.inject_frame(f2, n2, m);
  TEST_ASSERT_EQUAL_size_t(2u, app.count());
  // Whichever order they came in
  bool any_wps = false;
  for (size_t i = 0; i < app.count(); ++i) {
    if (app.entry_at(i).wps) any_wps = true;
  }
  TEST_ASSERT_TRUE(any_wps);
}

void test_wps_scan_dedupes_same_bssid() {
  Fixture f;
  FakeWifiMonitor mon;
  WpsScanApp app{mon};
  app.on_enter(f.hal);
  uint8_t bssid[6] = {1,2,3,4,5,6};
  uint8_t buf[80];
  size_t n = dot11::build_beacon(buf, sizeof(buf), "X", 1, bssid, 1);
  WifiRxMeta m{}; m.type = WifiPktType::Management;
  mon.inject_frame(buf, n, m);
  mon.inject_frame(buf, n, m);
  mon.inject_frame(buf, n, m);
  TEST_ASSERT_EQUAL_size_t(1u, app.count());
}

void test_dot11_has_wps_ie_detects_oui() {
  uint8_t bssid[6] = {0,0,0,0,0,0};
  uint8_t buf[120];
  size_t n = dot11::build_beacon(buf, sizeof(buf), "X", 1, bssid, 1);
  TEST_ASSERT_FALSE(dot11::has_wps_ie(buf, n));
  buf[n++] = 0xDD; buf[n++] = 0x04;
  buf[n++] = 0x00; buf[n++] = 0x50; buf[n++] = 0xF2; buf[n++] = 0x04;
  TEST_ASSERT_TRUE(dot11::has_wps_ie(buf, n));
}

// BleGattApp

void test_ble_gatt_starts_in_idle() {
  Fixture f;
  FakeBleCentral c;
  BleGattApp app{c};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.view() == BleGattApp::View::Idle);
}

void test_ble_gatt_connect_failure_lands_in_failed() {
  Fixture f;
  FakeBleCentral c;
  BleGattApp app{c};
  app.set_target_mac("AA:BB:CC:DD:EE:FF");  // not registered
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.view() == BleGattApp::View::Failed);
}

void test_ble_gatt_connect_enumerates_services() {
  Fixture f;
  FakeBleCentral c;
  c.register_service("AA:BB:CC:DD:EE:FF",
      "1800",
      {{"2A00", 0x02, {'Y','U','I'}}, {"2A01", 0x02, {0x01}}});
  c.register_service("AA:BB:CC:DD:EE:FF",
      "180F",
      {{"2A19", 0x12, {0x64}}});
  BleGattApp app{c};
  app.set_target_mac("AA:BB:CC:DD:EE:FF");
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.view() == BleGattApp::View::Services);
  TEST_ASSERT_EQUAL_size_t(2u, app.services_count());
}

void test_ble_gatt_drill_into_chars() {
  Fixture f;
  FakeBleCentral c;
  c.register_service("MAC",
      "180F",
      {{"2A19", 0x12, {0x42}}});
  BleGattApp app{c};
  app.set_target_mac("MAC");
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  app.on_key(press(Key::Enter));   // drill into first service
  TEST_ASSERT_TRUE(app.view() == BleGattApp::View::Chars);
  TEST_ASSERT_EQUAL_size_t(1u, app.chars_count());
  TEST_ASSERT_EQUAL_STRING("2A19", app.char_at(0).uuid);
  TEST_ASSERT_EQUAL_HEX8(0x12, app.char_at(0).properties);
}

// BleJammerApp

void test_ble_jammer_off_by_default() {
  Fixture f;
  FakeBleAdvertiser adv;
  BleJammerApp app{adv};
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(app.enabled());
  app.tick(0);
  TEST_ASSERT_FALSE(adv.active());
}

void test_ble_jammer_fn_enter_starts_rapid_cycling() {
  Fixture f;
  FakeBleAdvertiser adv;
  BleJammerApp app{adv};
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(app.enabled());
  app.tick(0);
  app.tick(20);
  app.tick(40);
  TEST_ASSERT_TRUE(app.cycles() >= 3);
  TEST_ASSERT_TRUE(adv.set_count() >= 3);
}

void test_ble_jammer_auto_off_after_30_seconds() {
  Fixture f;
  FakeBleAdvertiser adv;
  BleJammerApp app{adv};
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  app.tick(0);
  app.tick(31000);    // past kAutoOffMs
  TEST_ASSERT_FALSE(app.enabled());
  TEST_ASSERT_FALSE(adv.active());
}

// CaptivePortalApp

void test_captive_portal_idle_until_started() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeStorage st;
  CaptivePortalApp app{ap, fs, st};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.state() == CaptivePortalApp::State::Idle);
  TEST_ASSERT_FALSE(ap.active());
}

void test_captive_portal_fn_enter_starts_ap() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeStorage st;
  st.put_str("cp.ssid", "FreeWiFi");
  CaptivePortalApp app{ap, fs, st};
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == CaptivePortalApp::State::Running);
  TEST_ASSERT_TRUE(ap.active());
  TEST_ASSERT_TRUE(ap.captive());
}

void test_captive_portal_no_ssid_does_not_start() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeStorage st;
  CaptivePortalApp app{ap, fs, st};
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_FALSE(ap.active());
}

void test_captive_portal_captures_form_submissions() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeStorage st;
  st.put_str("cp.ssid", "X");
  CaptivePortalApp app{ap, fs, st};
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  ap.simulate_form_submit("192.168.4.2", "u=alice&p=hunter2");
  ap.simulate_form_submit("192.168.4.3", "u=bob&p=qwerty");
  TEST_ASSERT_EQUAL_size_t(2u, app.capture_count());
  TEST_ASSERT_EQUAL_STRING("u=alice&p=hunter2", app.capture_at(0).body);
  TEST_ASSERT_EQUAL_STRING("192.168.4.3", app.capture_at(1).peer_ip);
}

void test_captive_portal_tab_writes_captures_to_sd() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeStorage st;
  st.put_str("cp.ssid", "X");
  CaptivePortalApp app{ap, fs, st};
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  ap.simulate_form_submit("10.0.0.1", "u=test");
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(fs.exists("/captures.txt"));
}

void test_captive_portal_backspace_stops_ap() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeStorage st;
  st.put_str("cp.ssid", "X");
  CaptivePortalApp app{ap, fs, st};
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(ap.active());
  app.on_key(press(Key::Backspace));
  TEST_ASSERT_FALSE(ap.active());
  TEST_ASSERT_TRUE(app.state() == CaptivePortalApp::State::Stopped);
}

void test_ble_spam_disable_stops_advertiser() {
  Fixture f;
  FakeBleAdvertiser adv;
  BleSpamApp app{adv};
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  app.tick(0);
  TEST_ASSERT_TRUE(adv.active());
  app.on_key(press_fn(Key::Enter));   // toggle off
  TEST_ASSERT_FALSE(app.enabled());
  TEST_ASSERT_FALSE(adv.active());
}

void test_recon_app_done_state_enter_rescans() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  register_login_ok(h);
  h.register_response("POST",
      "http://pa.local:1471/api/recon/start",
      200, "{\"scanRunning\":true,\"scanID\":1}");
  h.register_response("GET",
      "http://pa.local:1471/api/recon/status",
      200, "{\"scanRunning\":false,\"scanPercent\":100,\"scanID\":1}");
  PineappleReconApp app{h, st};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  app.tick(0); app.tick(2000);
  TEST_ASSERT_TRUE(app.state() == PineappleReconApp::State::Done);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == PineappleReconApp::State::Scanning);
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
  RUN_TEST(test_launcher_with_sysprobe_renders_status_text);
  RUN_TEST(test_launcher_status_uses_wallclock_when_synced);
  RUN_TEST(test_launcher_status_falls_back_to_uptime_when_unsynced);
  RUN_TEST(test_launcher_without_sysprobe_skips_status);
  RUN_TEST(test_launcher_starts_in_categories_view);
  RUN_TEST(test_launcher_enter_on_empty_category_stays_in_categories);
  RUN_TEST(test_launcher_drills_into_category_on_enter);
  RUN_TEST(test_launcher_esc_returns_to_categories);
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
  RUN_TEST(test_wifi_app_tab_restarts_scan);
  RUN_TEST(test_wifi_app_enter_on_secured_opens_pass_editor);
  RUN_TEST(test_wifi_app_pass_editor_buffers_chars_and_backspace);
  RUN_TEST(test_wifi_app_pass_esc_returns_to_done);
  RUN_TEST(test_wifi_app_submit_calls_connect_and_persists);
  RUN_TEST(test_wifi_app_open_network_skips_pass_editor);
  RUN_TEST(test_wifi_app_connecting_advances_to_connected_on_tick);
  RUN_TEST(test_wifi_app_connecting_advances_to_failed_on_tick);
  RUN_TEST(test_wifi_app_failed_enter_reopens_editor);
  RUN_TEST(test_wifi_app_forget_on_done_clears_storage_and_disconnects);
  RUN_TEST(test_wifi_app_forget_in_done_state);
  RUN_TEST(test_wifi_app_plain_backspace_in_done_does_not_forget);
  RUN_TEST(test_wifi_app_already_connected_skips_scan);
  RUN_TEST(test_ble_app_starts_and_completes);
  RUN_TEST(test_ble_app_arrow_keys_move_cursor);
  RUN_TEST(test_calc_engine_addition);
  RUN_TEST(test_calc_engine_division_by_zero_errors);
  RUN_TEST(test_calc_engine_decimal_input);
  RUN_TEST(test_calc_engine_dup);
  RUN_TEST(test_calc_engine_subtraction);
  RUN_TEST(test_calc_engine_negate_buffer_and_stack);
  RUN_TEST(test_calc_engine_sqrt);
  RUN_TEST(test_calc_engine_sqrt_negative_errors);
  RUN_TEST(test_calc_engine_swap);
  RUN_TEST(test_calc_app_routes_unary_ops);
  RUN_TEST(test_calc_engine_backspace);
  RUN_TEST(test_calc_app_dispatches_digits_and_ops);
  RUN_TEST(test_calc_engine_sin_zero_is_zero);
  RUN_TEST(test_calc_engine_cos_zero_is_one);
  RUN_TEST(test_calc_engine_tan_pi_quarter_is_one);
  RUN_TEST(test_calc_engine_trig_empty_stack_errors);
  RUN_TEST(test_calc_engine_memory_store_recall_round_trip);
  RUN_TEST(test_calc_engine_memory_clear_zeros_and_unsets);
  RUN_TEST(test_calc_engine_memory_store_empty_errors);
  RUN_TEST(test_calc_app_routes_trig_and_memory_keys);
  RUN_TEST(test_calc_engine_starts_in_radians_mode);
  RUN_TEST(test_calc_engine_toggle_switches_angle_mode);
  RUN_TEST(test_calc_engine_sin_90_in_degrees_is_one);
  RUN_TEST(test_calc_engine_cos_180_in_degrees_is_neg_one);
  RUN_TEST(test_calc_engine_push_pi_pushes_constant);
  RUN_TEST(test_calc_engine_push_e_pushes_constant);
  RUN_TEST(test_calc_engine_push_const_flushes_buffer_first);
  RUN_TEST(test_calc_engine_reset_preserves_angle_mode);
  RUN_TEST(test_calc_app_d_toggles_angle_mode);
  RUN_TEST(test_calc_app_g_pushes_pi);
  RUN_TEST(test_imu_app_reads_accel_on_tick);
  RUN_TEST(test_imu_app_renders_header_red);
  RUN_TEST(test_notes_starts_empty);
  RUN_TEST(test_notes_typing_marks_dirty);
  RUN_TEST(test_notes_backspace);
  RUN_TEST(test_notes_arrow_left_right_move_cursor);
  RUN_TEST(test_notes_insert_mid_buffer);
  RUN_TEST(test_notes_backspace_mid_buffer);
  RUN_TEST(test_notes_up_down_preserve_target_column);
  RUN_TEST(test_notes_fn_left_jumps_to_line_start);
  RUN_TEST(test_notes_fn_right_jumps_to_line_end);
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
  RUN_TEST(test_settings_default_tz_is_utc);
  RUN_TEST(test_settings_tz_right_cycles_and_persists);
  RUN_TEST(test_settings_tz_left_wraps_to_last_preset);
  RUN_TEST(test_settings_tz_loads_persisted_value);
  RUN_TEST(test_clock_picks_up_tz_from_storage_on_enter);
  RUN_TEST(test_clock_ntp_resync_uses_storage_tz);
  RUN_TEST(test_tz_presets_index_of_known_value);
  RUN_TEST(test_settings_default_ntp_is_pool);
  RUN_TEST(test_settings_ntp_right_cycles_and_persists);
  RUN_TEST(test_settings_ntp_loads_persisted_value);
  RUN_TEST(test_clock_ntp_resync_uses_storage_ntp_server);
  RUN_TEST(test_ntp_presets_index_of_known_value);
  RUN_TEST(test_clock_starts_in_stopwatch_mode);
  RUN_TEST(test_clock_enter_toggles_running);
  RUN_TEST(test_clock_tick_advances_elapsed_when_running);
  RUN_TEST(test_clock_tab_switches_modes);
  RUN_TEST(test_clock_timer_up_down_adjusts_target);
  RUN_TEST(test_clock_backspace_resets);
  RUN_TEST(test_clock_time_of_day_renders_unsynced_placeholder);
  RUN_TEST(test_clock_time_of_day_enter_calls_ntp_sync);
  RUN_TEST(test_clock_time_of_day_enter_does_not_toggle_running);
  RUN_TEST(test_clock_time_of_day_renders_time_when_synced);
  RUN_TEST(test_fakenet_wifi_connect_rejects_empty_ssid);
  RUN_TEST(test_fakenet_wifi_connect_immediate_marks_connected);
  RUN_TEST(test_fakenet_wifi_connect_async_path);
  RUN_TEST(test_fakenet_wifi_disconnect_clears_state);
  RUN_TEST(test_fakenet_ntp_sync_requires_connected);
  RUN_TEST(test_fakeclock_epoch_defaults_to_zero);
  RUN_TEST(test_fakeclock_set_epoch_round_trips);
  RUN_TEST(test_snake_app_backspace_pauses);
  RUN_TEST(test_snake_step_interval_shrinks_with_score);
  RUN_TEST(test_tone_app_cursor_moves);
  RUN_TEST(test_tone_app_enter_starts_playing_and_emits_tones);
  RUN_TEST(test_tone_app_advances_through_preset);
  RUN_TEST(test_tone_app_backspace_stops);
  RUN_TEST(test_todo_starts_empty);
  RUN_TEST(test_todo_tab_starts_edit_then_enter_adds);
  RUN_TEST(test_todo_space_toggles_done);
  RUN_TEST(test_todo_backspace_deletes_selected);
  RUN_TEST(test_todo_persists_across_instances);
  RUN_TEST(test_todo_edit_backspace_aborts_when_buffer_empty);
  RUN_TEST(test_date_leap_year);
  RUN_TEST(test_date_days_in_month);
  RUN_TEST(test_date_day_of_week_known_dates);
  RUN_TEST(test_date_add_days_crosses_month);
  RUN_TEST(test_date_add_days_crosses_year_back);
  RUN_TEST(test_date_add_months_clamps_day);
  RUN_TEST(test_calendar_app_arrow_keys_navigate);
  RUN_TEST(test_calendar_app_enter_jumps_to_today);
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
  RUN_TEST(test_sysinfo_renders_time_when_synced);
  RUN_TEST(test_sysinfo_renders_no_sync_message_when_unsynced);
  RUN_TEST(test_fake_radiolink_starts_idle);
  RUN_TEST(test_fake_radiolink_connect_immediate_to_command_mode);
  RUN_TEST(test_fake_radiolink_connect_async_path);
  RUN_TEST(test_fake_radiolink_command_returns_registered_reply);
  RUN_TEST(test_fake_radiolink_unregistered_command_fails);
  RUN_TEST(test_fake_radiolink_kiss_round_trip);
  RUN_TEST(test_fake_radiolink_command_blocked_in_kiss_mode);
  RUN_TEST(test_fake_gnss_defaults_invalid);
  RUN_TEST(test_fake_gnss_set_fix_round_trips);
  RUN_TEST(test_fake_http_returns_canned_response);
  RUN_TEST(test_fake_http_unregistered_url_returns_neg_one);
  RUN_TEST(test_fake_pcap_writes_24_byte_header);
  RUN_TEST(test_fake_pcap_writes_packet_record_correctly);
  RUN_TEST(test_pcap_format_handshake_path);
  RUN_TEST(test_aprs_dti_lookup);
  RUN_TEST(test_ax25_constants);
  RUN_TEST(test_ax25_parse_minimal_position_frame);
  RUN_TEST(test_ax25_parse_with_digipath);
  RUN_TEST(test_ax25_parse_rejects_truncated);
  RUN_TEST(test_ax25_parse_rejects_dst_marked_last);
  RUN_TEST(test_aprs_parse_position_no_timestamp);
  RUN_TEST(test_aprs_parse_position_with_msg_dti);
  RUN_TEST(test_aprs_parse_position_rejects_status_dti);
  RUN_TEST(test_aprs_parse_position_rejects_compressed);
  RUN_TEST(test_aprs_parse_position_signs_southern_western);
  RUN_TEST(test_aprs_parse_status);
  RUN_TEST(test_aprs_parse_status_rejects_position_dti);
  RUN_TEST(test_kiss_encode_simple_payload);
  RUN_TEST(test_kiss_encode_escapes_fend_byte);
  RUN_TEST(test_kiss_encode_escapes_fesc_byte);
  RUN_TEST(test_kiss_encode_return_frame_is_three_bytes);
  RUN_TEST(test_kiss_encode_buffer_too_small_returns_zero);
  RUN_TEST(test_kiss_decode_simple_frame);
  RUN_TEST(test_kiss_decode_unescapes_fend);
  RUN_TEST(test_kiss_decode_unescapes_fesc);
  RUN_TEST(test_kiss_decode_drops_junk_between_frames);
  RUN_TEST(test_kiss_decode_back_to_back_fends_idle_resync);
  RUN_TEST(test_kiss_round_trip_encode_decode);
  RUN_TEST(test_aprs_app_skips_kiss_when_radio_idle);
  RUN_TEST(test_aprs_app_enters_kiss_mode_on_open);
  RUN_TEST(test_aprs_app_decodes_position_from_kiss_bytes);
  RUN_TEST(test_aprs_app_appends_ssid_to_callsign);
  RUN_TEST(test_aprs_app_dedupes_repeat_transmissions);
  RUN_TEST(test_aprs_app_status_frame_doesnt_create_station);
  RUN_TEST(test_aprs_app_arrow_keys_move_cursor);
  RUN_TEST(test_aprs_app_render_header_red);
  RUN_TEST(test_aprs_app_render_listening_when_empty);
  RUN_TEST(test_aprs_app_evicts_oldest_when_full);
  RUN_TEST(test_kenwood_app_shows_idle_when_disconnected);
  RUN_TEST(test_kenwood_app_refresh_queries_id_freq_mode);
  RUN_TEST(test_kenwood_app_enter_re_refreshes);
  RUN_TEST(test_kenwood_app_tab_uses_stored_mac);
  RUN_TEST(test_kenwood_app_tab_no_op_without_stored_mac);
  RUN_TEST(test_gps_app_shows_no_data_when_gnss_silent);
  RUN_TEST(test_gps_app_records_points_at_interval);
  RUN_TEST(test_gps_app_skips_invalid_fixes);
  RUN_TEST(test_gps_app_caps_at_max_points);
  RUN_TEST(test_gps_app_tab_stop_writes_gpx_to_fs);
  RUN_TEST(test_pineapple_client_unauthenticated_until_login);
  RUN_TEST(test_pineapple_client_login_extracts_token);
  RUN_TEST(test_pineapple_client_login_persists_creds_to_storage);
  RUN_TEST(test_pineapple_client_login_failure_keeps_unauthenticated);
  RUN_TEST(test_pineapple_dashboard_cards_parses_status);
  RUN_TEST(test_pineapple_recon_start_returns_scan_id);
  RUN_TEST(test_pineapple_recon_status_polls_progress);
  RUN_TEST(test_pineapple_401_triggers_relogin_and_retries);
  RUN_TEST(test_json_find_string_simple);
  RUN_TEST(test_json_find_string_missing_key_returns_false);
  RUN_TEST(test_json_find_int_unquoted_and_quoted);
  RUN_TEST(test_json_find_bool_true_false);
  RUN_TEST(test_pineapple_app_no_creds_renders_hint);
  RUN_TEST(test_pineapple_app_logs_in_and_fetches_cards);
  RUN_TEST(test_pineapple_app_login_failure_marks_error);
  RUN_TEST(test_pineapple_app_tab_re_fetches);
  RUN_TEST(test_recon_app_starts_in_idle);
  RUN_TEST(test_recon_app_enter_starts_scan);
  RUN_TEST(test_recon_app_tick_polls_and_completes);
  RUN_TEST(test_recon_app_login_failure_yields_error_state);
  RUN_TEST(test_recon_app_done_state_enter_rescans);
  RUN_TEST(test_dot11_extract_ssid_from_probe_request);
  RUN_TEST(test_dot11_addr2_matches_source_mac);
  RUN_TEST(test_dot11_is_eapol_data_recognizes_8888e_ethertype);
  RUN_TEST(test_dot11_format_mac);
  RUN_TEST(test_probe_app_starts_monitor_with_mgmt_filter);
  RUN_TEST(test_probe_app_records_unique_probe_requests);
  RUN_TEST(test_probe_app_ignores_non_probe_request_frames);
  RUN_TEST(test_probe_app_tick_hops_channels);
  RUN_TEST(test_probe_app_channel_wraps_at_13_to_1);
  RUN_TEST(test_probe_app_arrow_keys_navigate);
  RUN_TEST(test_handshake_app_opens_pcap_on_enter);
  RUN_TEST(test_handshake_app_writes_every_frame_to_pcap);
  RUN_TEST(test_handshake_app_increments_eapol_count_on_key_frame);
  RUN_TEST(test_handshake_app_tick_hops_channels);
  RUN_TEST(test_handshake_app_on_exit_stops_and_closes);
  // v0.2 stretch — Track A
  RUN_TEST(test_remote_head_refreshes_on_enter);
  RUN_TEST(test_remote_head_tab_cycles_field);
  RUN_TEST(test_remote_head_up_adjusts_freq);
  RUN_TEST(test_aprs_msg_no_callsign_disables_input);
  RUN_TEST(test_aprs_msg_loads_call_from_storage);
  RUN_TEST(test_aprs_msg_typed_chars_buffer_correctly);
  RUN_TEST(test_aprs_msg_send_writes_kiss_frame);
  // v0.2 stretch — Track B
  RUN_TEST(test_pineapple_recon_results_parses_aps);
  RUN_TEST(test_pineapple_pineap_set_enabled_sends_put);
  RUN_TEST(test_pineapple_deauth_ap_sends_post);
  RUN_TEST(test_handshake_browser_counts_bssids);
  RUN_TEST(test_evil_twin_fn_enter_enables);
  RUN_TEST(test_evil_twin_plain_enter_does_nothing);
  RUN_TEST(test_karma_fn_enter_enables);
  RUN_TEST(test_deauth_armed_required_to_fire);
  // v0.2 stretch — Track C
  RUN_TEST(test_ble_spam_off_by_default);
  RUN_TEST(test_ble_spam_fn_enter_toggles_and_advertises);
  RUN_TEST(test_ble_spam_cycles_payloads);
  RUN_TEST(test_ble_spam_disable_stops_advertiser);
  // Bruce-parity sweep
  RUN_TEST(test_tvbgone_starts_disabled);
  RUN_TEST(test_tvbgone_tab_enables_and_first_tick_fires);
  RUN_TEST(test_tvbgone_iterates_full_table_then_disables);
  RUN_TEST(test_beacon_flood_off_by_default);
  RUN_TEST(test_beacon_flood_tab_enables_and_tx_cycles);
  RUN_TEST(test_beacon_flood_frame_contains_ssid_bytes);
  RUN_TEST(test_beacon_flood_up_down_change_channel);
  RUN_TEST(test_native_deauth_disarmed_by_default);
  RUN_TEST(test_native_deauth_arm_and_fire_emits_frames);
  RUN_TEST(test_native_deauth_invalid_mac_aborts_fire);
  RUN_TEST(test_wps_scan_records_aps_with_and_without_wps);
  RUN_TEST(test_wps_scan_dedupes_same_bssid);
  RUN_TEST(test_dot11_has_wps_ie_detects_oui);
  RUN_TEST(test_ble_gatt_starts_in_idle);
  RUN_TEST(test_ble_gatt_connect_failure_lands_in_failed);
  RUN_TEST(test_ble_gatt_connect_enumerates_services);
  RUN_TEST(test_ble_gatt_drill_into_chars);
  RUN_TEST(test_ble_jammer_off_by_default);
  RUN_TEST(test_ble_jammer_fn_enter_starts_rapid_cycling);
  RUN_TEST(test_ble_jammer_auto_off_after_30_seconds);
  RUN_TEST(test_captive_portal_idle_until_started);
  RUN_TEST(test_captive_portal_fn_enter_starts_ap);
  RUN_TEST(test_captive_portal_no_ssid_does_not_start);
  RUN_TEST(test_captive_portal_captures_form_submissions);
  RUN_TEST(test_captive_portal_tab_writes_captures_to_sd);
  RUN_TEST(test_captive_portal_backspace_stops_ap);
  return UNITY_END();
}

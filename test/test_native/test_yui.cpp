// Native unit tests — pure logic, no hardware.
// Run with: pio test -e native

#include <unity.h>
#include "yui/shell/Menu.hpp"
#include "yui/types.hpp"
#include "yui/gfx/Braille.hpp"
#include "yui/gfx/koi_art.hpp"
#include "yui/shell/Splash.hpp"
#include "yui/shell/BootAnimation.hpp"
#include "../../src/hal/native/NativeDisplay.hpp"
#include "yui/app/AppRegistry.hpp"
#include "yui/app/Launcher.hpp"
#include "yui/app/AboutApp.hpp"
#include "yui/app/RemoteApp.hpp"
#include "yui/app/StubApp.hpp"
#include "yui/dsp/Fft.hpp"
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
#include "yui/app/FilesApp.hpp"
#include "yui/app/IrRemoteApp.hpp"
#include "yui/app/SettingsApp.hpp"
#include "yui/app/SysinfoApp.hpp"
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
#include "../../src/hal/native/NativeWifiMonitor.hpp"
#include "yui/app/WifiProbeApp.hpp"
#include "yui/app/WifiHandshakeApp.hpp"
#include "yui/app/RemoteHeadApp.hpp"
#include "yui/app/AprsMessageApp.hpp"
#include "yui/app/RogueApApp.hpp"
#include "yui/app/DeauthApp.hpp"
#include "yui/app/BleSpamApp.hpp"
#include "../../src/hal/native/NativeBleAdvertiser.hpp"
#include "../../src/hal/native/NativeBleRawTx.hpp"
#include "yui/app/MousejackApp.hpp"
#include "yui/app/WifiBeaconFloodApp.hpp"
#include "yui/app/WpsScanApp.hpp"
#include "yui/app/BleGattApp.hpp"
#include "yui/app/BleJammerApp.hpp"
#include "../../src/hal/native/NativeWifiAp.hpp"
#include "../../src/hal/native/NativeBleCentral.hpp"
#include "yui/sat/Tle.hpp"
#include "yui/sat/Time.hpp"
#include "yui/sat/Vec3.hpp"
#include "yui/sat/Propagator.hpp"
#include "yui/sat/Topo.hpp"
#include "yui/sat/Pass.hpp"
#include "yui/app/SatTrackerApp.hpp"
#include "yui/proto/Dot11.hpp"
#include "../../src/hal/native/NativeSpeaker.hpp"
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

void test_boot_animation_indexes_clamp_to_last_frame() {
  // First frame at t=0.
  TEST_ASSERT_EQUAL(0u, yui::boot_frame_index_at(0));
  // Mid-animation: frame N at N * kBootFrameMs ms.
  TEST_ASSERT_EQUAL(5u, yui::boot_frame_index_at(yui::kBootFrameMs * 5));
  // Past the end: clamped to the final frame.
  const std::size_t last = yui::assets::kBootFrameCount - 1;
  TEST_ASSERT_EQUAL(last,
                    yui::boot_frame_index_at(60u * 1000u));
}

void test_boot_animation_calls_draw_png_per_frame() {
  NativeDisplay d;
  // NativeDisplay's draw_png is the IDisplay default (no-op), so we just
  // assert clear+flush wiring is intact and the call doesn't crash.
  yui::render_boot_animation(d, 0);
  TEST_ASSERT_EQUAL(1, d.flush_count());
  yui::render_boot_animation(d, yui::kBootFrameMs * 10);
  TEST_ASSERT_EQUAL(2, d.flush_count());
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

void test_launcher_left_right_flip_categories() {
  Fixture f;
  Launcher l{f.registry};
  l.on_enter(f.hal);
  const Category start = l.current_category();
  l.on_key(press(Key::Right));
  TEST_ASSERT_TRUE(l.current_category() != start);
  l.on_key(press(Key::Left));
  TEST_ASSERT_TRUE(l.current_category() == start);
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

// ───── RemoteApp + Fn+V chord ───────────────────────────────────────────────

namespace {
class MockRemote : public IRemote {
public:
  bool start() override     { running_ = true; ++start_calls; return true; }
  void stop() override      { running_ = false; ++stop_calls; }
  bool running() const override { return running_; }
  const char* ip() const override { return "10.0.0.42"; }
  bool poll_key(KeyEvent&) override { return false; }
  int  start_calls = 0;
  int  stop_calls  = 0;
private:
  bool running_ = false;
};
}  // namespace

void test_remote_app_enter_toggles_runtime_only() {
  Fixture f;
  MockRemote remote;
  FakeStorage store;
  store.init();
  RemoteApp app{remote, &store};
  app.on_enter(f.hal);

  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(remote.running());
  TEST_ASSERT_EQUAL(1, remote.start_calls);

  app.on_key(press(Key::Enter));
  TEST_ASSERT_FALSE(remote.running());
  TEST_ASSERT_EQUAL(1, remote.stop_calls);

  // Persist flag untouched by Enter — that lever is F. get_int returns
  // false when the key is absent, which is exactly what we want here.
  int32_t persisted = 99;
  const bool present = store.get_int(kStorageKeyDevRemote, persisted, 0);
  TEST_ASSERT_FALSE(present);
}

void test_remote_app_f_flips_persist_flag() {
  Fixture f;
  MockRemote remote;
  FakeStorage store;
  store.init();
  RemoteApp app{remote, &store};
  app.on_enter(f.hal);

  KeyEvent fk{};
  fk.key = Key::Char; fk.ch = 'f'; fk.down = true;
  app.on_key(fk);
  int32_t v = -1;
  store.get_int(kStorageKeyDevRemote, v, -1);
  TEST_ASSERT_EQUAL(1, v);
  TEST_ASSERT_EQUAL(1, app.persist_flag());

  app.on_key(fk);
  store.get_int(kStorageKeyDevRemote, v, -1);
  TEST_ASSERT_EQUAL(0, v);
}

void test_shell_fn_plus_v_toggles_remote_and_persists() {
  Fixture f;
  MockRemote remote;
  FakeStorage store;
  store.init();
  StubApp stub{"x"};
  f.registry.add(&stub);
  Launcher l{f.registry};
  Shell s{f.hal, l, "v-test", 0};  // splash min = 0 so we drop straight in
  s.bind_remote(&remote, &store);
  s.start();

  // Tick once to leave splash on a key press.
  f.keyboard.inject(press(Key::Enter));
  s.tick();

  KeyEvent chord{};
  chord.key = Key::Char; chord.ch = 'v'; chord.fn = true; chord.down = true;
  f.keyboard.inject(chord);
  s.tick();
  TEST_ASSERT_TRUE(remote.running());
  int32_t v = -1;
  store.get_int(kStorageKeyDevRemote, v, -1);
  TEST_ASSERT_EQUAL(1, v);
  TEST_ASSERT_TRUE(s.last_chord_toggled());

  f.keyboard.inject(chord);
  s.tick();
  TEST_ASSERT_FALSE(remote.running());
  store.get_int(kStorageKeyDevRemote, v, -1);
  TEST_ASSERT_EQUAL(0, v);
}

void test_launcher_renders_carousel_tile_in_red() {
  Fixture f;
  StubApp a{"A"};
  f.registry.add(&a);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  l.render(f.display);
  // Carousel: full white background (no header bar), with a red icon tile
  // centered horizontally near the top. Tile is 56×56 starting at y≈14.
  // Tile is 56-px wide, centered → on a 240-wide display the tile occupies
  // x=92..147. Sample (95, 40): inside the red backplate, away from any
  // white glyph stroke at the center.
  TEST_ASSERT_EQUAL_HEX16(kWhite,    f.display.pixel_at(2, 5));
  TEST_ASSERT_EQUAL_HEX16(kJapanRed, f.display.pixel_at(95, 40));
}

void test_launcher_renders_dot_pager_for_current_category() {
  Fixture f;
  StubApp a{"A"};
  f.registry.add(&a);
  Launcher l{f.registry};
  l.on_enter(f.hal);
  l.render(f.display);
  // Bottom-row pager: the cursor's dot is filled red, others are dark-red
  // mini-squares. Cursor starts at index 0 (Radio), so the leftmost pager
  // pixel should be kJapanRed.
  const int H = f.display.height();
  // The leftmost pager dot lives at x = pager_x; sample y = H-8 row.
  // We just need to confirm that *some* red pixel exists on the pager row,
  // and that pixel(0..1, H-1) is white (no full-width strip).
  bool found_red = false;
  for (int x = 0; x < f.display.width(); ++x) {
    if (f.display.pixel_at(x, H - 8) == kJapanRed) { found_red = true; break; }
  }
  TEST_ASSERT_TRUE(found_red);
  TEST_ASSERT_EQUAL_HEX16(kWhite, f.display.pixel_at(0, H - 1));
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

void test_adv_arrow_keys_are_primary() {
  // After the v1.x keymap rework, arrows are the primary action on the
  // ; , . / cluster (OS is nav-driven). Pressing the ',' position with
  // no modifiers should yield Left; Fn+, gives the literal comma.
  KeyEvent e{};
  TEST_ASSERT_TRUE(adv_make_event(54, true, false, false, false, false, e));
  TEST_ASSERT_TRUE(e.key == Key::Left);

  KeyEvent e2{};
  TEST_ASSERT_TRUE(adv_make_event(54, true, false, true, false, false, e2));
  TEST_ASSERT_TRUE(e2.key == Key::Char);
  TEST_ASSERT_EQUAL_CHAR(',', e2.ch);
}

void test_adv_esc_is_primary_on_backtick_key() {
  // The ` key (row 0 col 0) used to print backtick by default; Esc
  // required Fn. Now Esc is primary, ` is on the Fn layer. Keycode 1
  // = raw_row 0, raw_col 0 → logical (0,0).
  KeyEvent e{};
  TEST_ASSERT_TRUE(adv_make_event(1, true, false, false, false, false, e));
  TEST_ASSERT_TRUE(e.key == Key::Esc);
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

void test_ntp_presets_index_of_known_value() {
  TEST_ASSERT_EQUAL_size_t(0u, ntp_index_of("pool.ntp.org"));
  TEST_ASSERT_TRUE(ntp_index_of("time.google.com") > 0u);
  TEST_ASSERT_EQUAL_size_t(0u, ntp_index_of("not-a-server"));
  TEST_ASSERT_EQUAL_size_t(0u, ntp_index_of(nullptr));
}

void test_tz_presets_index_of_known_value() {
  TEST_ASSERT_EQUAL_size_t(0u, tz_index_of("UTC0"));
  TEST_ASSERT_TRUE(tz_index_of("JST-9") > 0u);
  TEST_ASSERT_EQUAL_size_t(0u, tz_index_of("not-a-real-tz"));
  TEST_ASSERT_EQUAL_size_t(0u, tz_index_of(nullptr));
}

// ───── ClockApp ─────────────────────────────────────────────────────────────

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

void test_pineapple_app_enter_re_fetches() {
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
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(h.calls() > calls_before);
}

void test_recon_app_starts_in_idle() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  PineappleApp app{h, st};
  app.on_enter(f.hal);
  app.set_tab(PineappleApp::Tab::Recon);
  TEST_ASSERT_TRUE(app.state() == PineappleApp::ReconState::Idle);
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
  PineappleApp app{h, st};
  app.on_enter(f.hal);
  app.set_tab(PineappleApp::Tab::Recon);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == PineappleApp::ReconState::Scanning);
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
  PineappleApp app{h, st};
  app.on_enter(f.hal);
  app.set_tab(PineappleApp::Tab::Recon);
  app.on_key(press(Key::Enter));
  app.tick(0);
  app.tick(2000);
  TEST_ASSERT_TRUE(app.state() == PineappleApp::ReconState::Done);
  TEST_ASSERT_EQUAL_INT(100, app.percent());
}

void test_recon_app_login_failure_yields_error_state() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  PineappleApp app{h, st};
  app.on_enter(f.hal);
  app.set_tab(PineappleApp::Tab::Recon);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == PineappleApp::ReconState::Error);
}

void test_pineapple_tab_cycles_through_three_tabs() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  PineappleApp app{h, st};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.tab() == PineappleApp::Tab::Dashboard);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.tab() == PineappleApp::Tab::Recon);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.tab() == PineappleApp::Tab::Handshakes);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.tab() == PineappleApp::Tab::Dashboard);
}

void test_pineapple_no_creds_renders_unreachable_consistently() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  PineappleApp app{h, st};
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(app.last_ok());
  app.set_tab(PineappleApp::Tab::Handshakes);
  TEST_ASSERT_FALSE(app.handshakes_ok());
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

// ───── Theme (now in SettingsApp row 3) ────────────────────────────────────

void test_theme_default_is_kohaku() {
  // Defaults established in Tokens.hpp.
  TEST_ASSERT_EQUAL_HEX16(yui::ui::kKohaku.accent, yui::ui::kAccent);
}

void test_theme_apply_palette_swaps_live_tokens() {
  yui::ui::apply_palette(yui::ui::kOgon);
  TEST_ASSERT_EQUAL_HEX16(yui::ui::kOgon.accent, yui::ui::kAccent);
  TEST_ASSERT_EQUAL_HEX16(yui::ui::kOgon.warn,   yui::ui::kWarn);
  TEST_ASSERT_EQUAL_HEX16(yui::ui::kOgon.accent_dark, yui::ui::kHint);
  yui::ui::apply_palette(yui::ui::kKohaku);
}

void test_settings_theme_row_persists_selection() {
  Fixture f;
  FakeStorage store; store.init();
  SettingsApp app{store};
  app.on_enter(f.hal);
  app.set_cursor(3);  // Theme row
  // Step right once → next palette (ogon at index 1).
  app.on_key(press(Key::Right));
  char id[16] = {0};
  TEST_ASSERT_TRUE(store.get_str(kStorageKeyTheme, id, sizeof(id)));
  TEST_ASSERT_EQUAL_STRING("ogon", id);
  TEST_ASSERT_EQUAL_HEX16(yui::ui::kOgon.accent, yui::ui::kAccent);
  yui::ui::apply_palette(yui::ui::kKohaku);
}

void test_settings_theme_row_loads_persisted_palette_on_enter() {
  Fixture f;
  FakeStorage store; store.init();
  store.put_str(kStorageKeyTheme, "asagi");
  SettingsApp app{store};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL(2u, app.theme_index());
  TEST_ASSERT_EQUAL_STRING("asagi", app.theme_id());
}

// ───── Phase 5.0 — Faraday Mode flag ───────────────────────────────────────

void test_faraday_default_off_after_load() {
  FakeStorage store; store.init();
  FaradayMode::reset_for_test();
  FaradayMode::load(store);
  TEST_ASSERT_FALSE(FaradayMode::is_active());
}

void test_faraday_enable_persists_to_nvs_and_reloads() {
  FakeStorage store; store.init();
  FaradayMode::reset_for_test();
  TEST_ASSERT_TRUE(FaradayMode::enable(store));
  TEST_ASSERT_TRUE(FaradayMode::is_active());
  // Wipe in-memory state, reload from NVS — should still be on.
  FaradayMode::reset_for_test();
  FaradayMode::load(store);
  TEST_ASSERT_TRUE(FaradayMode::is_active());
}

void test_faraday_disable_clears_persisted_state() {
  FakeStorage store; store.init();
  FaradayMode::reset_for_test();
  FaradayMode::enable(store);
  FaradayMode::disable(store);
  TEST_ASSERT_FALSE(FaradayMode::is_active());
  FaradayMode::reset_for_test();
  FaradayMode::load(store);
  TEST_ASSERT_FALSE(FaradayMode::is_active());
}

void test_faraday_gps_guard_refuses_when_far_from_lab() {
  FakeStorage store; store.init();
  FaradayMode::reset_for_test();
  // Lab at 0,0; GPS fix at 1°N (~111 km away) → way beyond 50 m guard.
  FaradayMode::set_lab_location(store, 0.0, 0.0);
  TEST_ASSERT_TRUE(FaradayMode::has_lab_location());
  TEST_ASSERT_FALSE(FaradayMode::enable(store, /*has_fix=*/true,
                                         /*lat=*/1.0, /*lon=*/0.0));
  TEST_ASSERT_FALSE(FaradayMode::is_active());
}

void test_faraday_gps_guard_allows_inside_lab_radius() {
  FakeStorage store; store.init();
  FaradayMode::reset_for_test();
  FaradayMode::set_lab_location(store, 0.0, 0.0);
  // Move ~10 m east of (0,0) — well within 50 m.
  TEST_ASSERT_TRUE(FaradayMode::enable(store, /*has_fix=*/true,
                                        0.0, 0.00009));
  TEST_ASSERT_TRUE(FaradayMode::is_active());
}

void test_faraday_no_fix_succeeds_even_with_lab_set() {
  FakeStorage store; store.init();
  FaradayMode::reset_for_test();
  FaradayMode::set_lab_location(store, 47.6, -122.3);
  // No GPS fix available → guard short-circuits and allows enable.
  TEST_ASSERT_TRUE(FaradayMode::enable(store, /*has_fix=*/false));
  TEST_ASSERT_TRUE(FaradayMode::is_active());
}

// ───── C1 — BbLinkProbeApp ─────────────────────────────────────────────

#include "yui/app/BbLinkProbeApp.hpp"

namespace {

yui::BleDevice make_ble_dev(const char* name, const char* addr) {
  yui::BleDevice d{};
  std::strncpy(d.name, name, sizeof(d.name) - 1);
  std::strncpy(d.addr, addr, sizeof(d.addr) - 1);
  d.rssi = -55;
  return d;
}

}  // namespace

void test_bblink_probe_pending_until_run() {
  Fixture f;
  FakeNet net;
  yui::FakeBleCentral central;
  yui::BbLinkProbeApp app{net, central};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.result_at(0) == yui::BbLinkProbeApp::Result::Pending);
  TEST_ASSERT_FALSE(app.bridge_ready());
}

void test_bblink_probe_no_advertise_marks_scan_fail() {
  Fixture f;
  FakeNet net;
  net.set_ble_results({});  // empty
  net.simulate_ble_done();
  yui::FakeBleCentral central;
  yui::BbLinkProbeApp app{net, central};
  app.on_enter(f.hal);
  app.run_for_test();
  TEST_ASSERT_TRUE(app.result_at(0) == yui::BbLinkProbeApp::Result::Fail);
  TEST_ASSERT_FALSE(app.bridge_ready());
}

void test_bblink_probe_full_path_passes_all_three() {
  Fixture f;
  FakeNet net;
  net.set_ble_results({
    make_ble_dev("OtherDev",  "00:00:00:00:00:01"),
    make_ble_dev("B.B. Link", "AA:BB:CC:DD:EE:FF"),
  });
  net.simulate_ble_done();
  yui::FakeBleCentral central;
  central.register_service(
    "AA:BB:CC:DD:EE:FF",
    "6E400001-B5A3-F393-E0A9-E50E24DCCA9E",
    {});
  yui::BbLinkProbeApp app{net, central};
  app.on_enter(f.hal);
  app.run_for_test();
  TEST_ASSERT_TRUE(app.bridge_ready());
}

void test_bblink_probe_missing_nus_marks_gatt_fail() {
  Fixture f;
  FakeNet net;
  net.set_ble_results({make_ble_dev("B.B. Link", "AA:BB:CC:DD:EE:FF")});
  net.simulate_ble_done();
  yui::FakeBleCentral central;
  // Register a connection but no NUS service.
  central.register_service("AA:BB:CC:DD:EE:FF",
                           "0000180A-0000-1000-8000-00805F9B34FB", {});
  yui::BbLinkProbeApp app{net, central};
  app.on_enter(f.hal);
  app.run_for_test();
  TEST_ASSERT_TRUE(app.result_at(0) == yui::BbLinkProbeApp::Result::Pass);
  TEST_ASSERT_TRUE(app.result_at(1) == yui::BbLinkProbeApp::Result::Pass);
  TEST_ASSERT_TRUE(app.result_at(2) == yui::BbLinkProbeApp::Result::Fail);
}

// ───── B2 — Boot self-test mode (NVS flag) ─────────────────────────────

void test_settings_boot_selftest_default_off() {
  Fixture f;
  FakeStorage st; st.init();
  SettingsApp app{st};
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(app.boot_selftest_enabled());
}

void test_settings_boot_selftest_toggle_persists() {
  Fixture f;
  FakeStorage st; st.init();
  {
    SettingsApp app{st};
    app.on_enter(f.hal);
    app.set_cursor(5);
    app.on_key(press(Key::Right));
    TEST_ASSERT_TRUE(app.boot_selftest_enabled());
  }
  // Re-enter — value loaded from NVS.
  {
    SettingsApp app{st};
    app.on_enter(f.hal);
    TEST_ASSERT_TRUE(app.boot_selftest_enabled());
  }
}

// ───── B1 — SelfTestApp ────────────────────────────────────────────────

#include "yui/app/SelfTestApp.hpp"

void test_selftest_initial_state_pending() {
  Fixture f;
  yui::SelfTestApp::Wiring w{};
  yui::SelfTestApp app{w};
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(app.started());
  for (int i = 0; i < yui::SelfTestApp::kProbeCount; ++i) {
    TEST_ASSERT_TRUE(app.result_at(i) == yui::SelfTestApp::Result::Pending);
  }
}

void test_selftest_run_with_no_wiring_marks_skip_for_optional() {
  Fixture f;
  yui::SelfTestApp::Wiring w{};
  yui::SelfTestApp app{w};
  app.on_enter(f.hal);
  app.run_for_test();
  TEST_ASSERT_TRUE(app.started());
  // Display, Keyboard, Clock, AppRegistry headroom always pass with hal.
  // FS/Storage/Net/BLE/IR/IMU/Mic/Speaker/CC1101/nRF24 should be Skip.
  TEST_ASSERT_TRUE(app.skip_count() >= 9);
  TEST_ASSERT_EQUAL_INT(0, app.fail_count());
}

void test_selftest_full_wiring_passes_all_present() {
  Fixture f;
  FakeFs fs;
  FakeStorage st;
  yui::FakeBleAdvertiser ble;
  yui::SelfTestApp::Wiring w{};
  w.fs = &fs; w.store = &st; w.ble = &ble;
  yui::SelfTestApp app{w};
  app.on_enter(f.hal);
  app.run_for_test();
  TEST_ASSERT_EQUAL_INT(0, app.fail_count());
  TEST_ASSERT_TRUE(app.pass_count() >= 6);
}

void test_selftest_storage_rw_round_trip() {
  Fixture f;
  FakeStorage st;
  yui::SelfTestApp::Wiring w{};
  w.store = &st;
  yui::SelfTestApp app{w};
  app.on_enter(f.hal);
  app.run_for_test();
  // Storage probe is index 3 in the probes array.
  TEST_ASSERT_TRUE(app.result_at(3) == yui::SelfTestApp::Result::Pass);
  // Confirm the probe wrote + read the sentinel key.
  int32_t v = 0;
  TEST_ASSERT_TRUE(st.get_int("sys.selftest_ping", v, 0));
  TEST_ASSERT_EQUAL_INT(1, v);
}

void test_selftest_cap_absent_marks_skip_not_fail() {
  Fixture f;
  // NativeCc1101 isn't included this early in the file; instead use a
  // minimal local stub that pretends absence.
  struct AbsentCc : yui::ICc1101 {
    bool begin() override { return false; }
    bool is_present() override { return false; }
    bool set_frequency_hz(uint32_t) override { return true; }
    uint32_t frequency_hz() const override { return 0; }
    bool set_modulation(yui::CcModulation) override { return true; }
    bool set_bitrate_bps(uint32_t) override { return true; }
    int16_t read_rssi_dbm() override { return -100; }
    int  receive(uint8_t*, std::size_t) override { return 0; }
    bool transmit(const uint8_t*, std::size_t) override { return true; }
    bool set_carrier(bool) override { return true; }
    bool transmit_raw_edges(const int32_t*, std::size_t) override { return true; }
    uint64_t rx_bytes() const override { return 0; }
    uint64_t tx_bytes() const override { return 0; }
  } cc;
  yui::SelfTestApp::Wiring w{};
  w.cc = &cc;
  yui::SelfTestApp app{w};
  app.on_enter(f.hal);
  app.run_for_test();
  TEST_ASSERT_TRUE(app.result_at(11) == yui::SelfTestApp::Result::Skip);
  TEST_ASSERT_EQUAL_INT(0, app.fail_count());
}

// ───── Phase 5.2.x — BLE Rail picker integration ───────────────────────

void test_blespam_default_rail_is_nimble() {
  Fixture f;
  FakeBleAdvertiser adv;
  BleSpamApp app{adv};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.rail() == BleSpamApp::Rail::Nimble);
}

void test_blespam_set_nrf24_rail_promotes_to_both() {
  Fixture f;
  FakeBleAdvertiser adv;
  yui::FakeNrf24BleRawTx nrf24;
  BleSpamApp app{adv};
  app.set_nrf24_rail(&nrf24);
  TEST_ASSERT_TRUE(app.rail() == BleSpamApp::Rail::Both);
}

void test_blespam_dual_rail_drives_both_engines() {
  Fixture f;
  FakeBleAdvertiser adv;
  yui::FakeNrf24BleRawTx nrf24;
  BleSpamApp app{adv};
  app.set_nrf24_rail(&nrf24);
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));   // enable
  app.tick(0);
  app.tick(500);
  TEST_ASSERT_TRUE(adv.set_count() >= 1);
  TEST_ASSERT_TRUE(nrf24.tx_count() >= 1);
}

void test_blespam_tab_cycles_rail_when_idle() {
  Fixture f;
  FakeBleAdvertiser adv;
  yui::FakeNrf24BleRawTx nrf24;
  BleSpamApp app{adv};
  app.set_nrf24_rail(&nrf24);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.rail() == BleSpamApp::Rail::Both);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.rail() == BleSpamApp::Rail::Nimble);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.rail() == BleSpamApp::Rail::Nrf24);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.rail() == BleSpamApp::Rail::Both);
}

void test_blejammer_dual_rail_starts_continuous() {
  Fixture f;
  FakeBleAdvertiser adv;
  yui::FakeNrf24BleRawTx nrf24;
  BleJammerApp app{adv};
  app.set_nrf24_rail(&nrf24);
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  app.tick(0);
  app.tick(20);
  TEST_ASSERT_TRUE(adv.active());
  TEST_ASSERT_TRUE(nrf24.is_active());
  TEST_ASSERT_EQUAL_INT(IBleRawTx::kChannel39, nrf24.channel());
}

void test_blejammer_disable_stops_both_rails() {
  Fixture f;
  FakeBleAdvertiser adv;
  yui::FakeNrf24BleRawTx nrf24;
  BleJammerApp app{adv};
  app.set_nrf24_rail(&nrf24);
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  app.tick(0); app.tick(20);
  app.on_key(press_fn(Key::Enter));   // toggle off
  TEST_ASSERT_FALSE(adv.active());
  TEST_ASSERT_FALSE(nrf24.is_active());
}

// ───── Phase 5.5 — Aggressive Mousejack + SubGhzBrute unlock ───────────

#include "yui/app/SubGhzBruteApp.hpp"

void test_mousejack_pace_clamps_when_faraday_off() {
  FaradayMode::reset_for_test();
  TEST_ASSERT_EQUAL_UINT32(50u, yui::MousejackApp::inject_pace_ms());
}

void test_mousejack_pace_unlocks_when_faraday_on() {
  FakeStorage st; st.init();
  FaradayMode::reset_for_test();
  FaradayMode::enable(st);
  TEST_ASSERT_EQUAL_UINT32(2u, yui::MousejackApp::inject_pace_ms());
  FaradayMode::reset_for_test();
}

void test_brute_pace_clamps_when_faraday_off() {
  FaradayMode::reset_for_test();
  TEST_ASSERT_EQUAL_UINT32(100u, yui::SubGhzBruteApp::step_pace_ms());
}

void test_brute_pace_unlocks_when_faraday_on() {
  FakeStorage st; st.init();
  FaradayMode::reset_for_test();
  FaradayMode::enable(st);
  TEST_ASSERT_EQUAL_UINT32(10u, yui::SubGhzBruteApp::step_pace_ms());
  FaradayMode::reset_for_test();
}

// ───── Phase 5.4 — PMKID capture mode for WifiHandshakeApp ─────────────

#include "yui/proto/Eapol.hpp"
#include "yui/app/WifiHandshakeApp.hpp"

namespace {

// Build a synthetic data frame that wraps an EAPOL body containing a
// PMKID KDE. Returns total length.
std::size_t build_eapol_with_pmkid(uint8_t* out, std::size_t cap,
                                    const uint8_t pmkid[16]) {
  if (cap < 200) return 0;
  std::memset(out, 0, 200);
  // 802.11 data header (24 bytes, non-QoS).
  out[0] = 0x08;             // type=Data, subtype=Data
  out[1] = 0x00;
  // Addresses: dst, src, bssid (24 bytes total: 4 + 6+6+6 + 2)
  for (int i = 0; i < 6; ++i) {
    out[4 + i]  = 0x11;      // addr1 = dst
    out[10 + i] = 0x22;      // addr2 = src
    out[16 + i] = 0x33;      // addr3 = bssid
  }
  // SeqCtl
  out[22] = 0; out[23] = 0;
  // LLC/SNAP + EAPOL ethertype
  std::size_t off = 24;
  out[off++] = 0xAA; out[off++] = 0xAA; out[off++] = 0x03;
  out[off++] = 0x00; out[off++] = 0x00; out[off++] = 0x00;
  out[off++] = 0x88; out[off++] = 0x8E;
  // EAPOL body: pad with zeros, then KDE marker
  for (int i = 0; i < 60; ++i) out[off++] = 0;
  // PMKID KDE: tag=0xDD, len=20, OUI 00:0F:AC, type=0x04, then 16-byte PMKID
  out[off++] = 0xDD;
  out[off++] = 20;
  out[off++] = 0x00; out[off++] = 0x0F; out[off++] = 0xAC; out[off++] = 0x04;
  std::memcpy(out + off, pmkid, 16);
  off += 16;
  return off;
}

}  // namespace

void test_eapol_m1_request_has_correct_layout() {
  uint8_t buf[128] = {0};
  std::size_t n = yui::eapol::build_m1_request(buf, sizeof(buf));
  TEST_ASSERT_EQUAL_size_t(106u, n);
  // LLC/SNAP
  TEST_ASSERT_EQUAL_HEX8(0xAA, buf[0]);
  TEST_ASSERT_EQUAL_HEX8(0xAA, buf[1]);
  TEST_ASSERT_EQUAL_HEX8(0x03, buf[2]);
  TEST_ASSERT_EQUAL_HEX8(0x88, buf[6]);
  TEST_ASSERT_EQUAL_HEX8(0x8E, buf[7]);
  // EAPOL header version=2, type=Key (0x03), length 95
  TEST_ASSERT_EQUAL_HEX8(0x02, buf[8]);
  TEST_ASSERT_EQUAL_HEX8(0x03, buf[9]);
  TEST_ASSERT_EQUAL_HEX8(95,   buf[11]);
  // Key descriptor type RSN (0x02)
  TEST_ASSERT_EQUAL_HEX8(0x02, buf[12]);
}

void test_eapol_extract_pmkid_finds_kde() {
  uint8_t pmkid[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
  uint8_t frame[200];
  std::size_t n = build_eapol_with_pmkid(frame, sizeof(frame), pmkid);
  // Skip MAC header (24) + LLC/SNAP (8) to land on EAPOL body.
  uint8_t out[16];
  TEST_ASSERT_TRUE(yui::eapol::extract_pmkid(frame + 32, n - 32, out));
  TEST_ASSERT_EQUAL_HEX8_ARRAY(pmkid, out, 16);
}

void test_eapol_extract_pmkid_returns_false_when_absent() {
  uint8_t body[120] = {0};
  uint8_t out[16];
  TEST_ASSERT_FALSE(yui::eapol::extract_pmkid(body, sizeof(body), out));
}

void test_handshake_app_tab_toggles_mode() {
  Fixture f;
  FakeWifiMonitor mon;
  FakePcap pcap;
  yui::WifiHandshakeApp app{mon, pcap, f.clock};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.mode() == yui::WifiHandshakeApp::Mode::FourWay);
  app.on_key(press(yui::Key::Tab));
  TEST_ASSERT_TRUE(app.mode() == yui::WifiHandshakeApp::Mode::PmkidOnly);
  app.on_key(press(yui::Key::Tab));
  TEST_ASSERT_TRUE(app.mode() == yui::WifiHandshakeApp::Mode::FourWay);
}

void test_handshake_app_counts_pmkid_in_injected_eapol() {
  Fixture f;
  FakeWifiMonitor mon;
  FakePcap pcap;
  yui::WifiHandshakeApp app{mon, pcap, f.clock};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(0, app.pmkid_seen());
  uint8_t pmkid[16] = {0xDE,0xAD,0xBE,0xEF,0,0,0,0,0,0,0,0,0,0,0,0};
  uint8_t frame[200];
  std::size_t n = build_eapol_with_pmkid(frame, sizeof(frame), pmkid);
  app.inject_for_test(frame, n);
  TEST_ASSERT_EQUAL_size_t(1, app.eapol_seen());
  TEST_ASSERT_EQUAL_size_t(1, app.pmkid_seen());
}

// ───── Phase 5.3 — RfChaosApp (lab-only RF stress test) ────────────────

#include "yui/app/RfChaosApp.hpp"

namespace {
struct ChaosCounter {
  int n = 0;
  void operator()() { ++n; }
};
}  // namespace

void test_rfchaos_refuses_arm_without_faraday() {
  Fixture f;
  FakeFs fs;
  yui::FakeClock clock;
  FaradayMode::reset_for_test();
  RfChaosApp app{fs, clock};
  ChaosCounter c;
  app.add_target({"deauth", std::ref(c), 6});
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == RfChaosApp::State::Refused);
  TEST_ASSERT_EQUAL_INT(0, c.n);
}

void test_rfchaos_arms_when_faraday_on_and_targets_present() {
  Fixture f;
  FakeFs fs;
  FakeStorage st; st.init();
  yui::FakeClock clock;
  FaradayMode::reset_for_test();
  FaradayMode::enable(st);
  RfChaosApp app{fs, clock};
  ChaosCounter c;
  app.add_target({"deauth", std::ref(c), 6});
  app.on_enter(f.hal);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == RfChaosApp::State::Armed);
  FaradayMode::reset_for_test();
}

void test_rfchaos_fire_rate_scales_with_intensity() {
  // Intensity 10 → 100ms interval → ~10 fires in 1s. Intensity 1 →
  // 1000ms interval → ~1 fire in 1s.
  Fixture f;
  FakeFs fs;
  FakeStorage st; st.init();
  yui::FakeClock clock;
  FaradayMode::reset_for_test(); FaradayMode::enable(st);
  RfChaosApp app{fs, clock};
  ChaosCounter c;
  app.add_target({"a", std::ref(c), 0});
  app.on_enter(f.hal);
  app.set_intensity(10);
  app.set_seed(1);
  app.on_key(press_fn(Key::Enter));
  for (uint32_t t = 0; t < 1000; t += 50) app.tick(t);
  TEST_ASSERT_TRUE(c.n >= 8);
  FaradayMode::reset_for_test();
}

void test_rfchaos_natural_completion_at_deadline() {
  Fixture f;
  FakeFs fs;
  FakeStorage st; st.init();
  yui::FakeClock clock;
  FaradayMode::reset_for_test(); FaradayMode::enable(st);
  RfChaosApp app{fs, clock};
  ChaosCounter c;
  app.add_target({"a", std::ref(c), 0});
  app.on_enter(f.hal);
  app.set_duration(RfChaosApp::Duration::OneMin);
  app.set_seed(7);
  app.on_key(press_fn(Key::Enter));
  app.tick(60'000 + 1);  // past deadline
  TEST_ASSERT_TRUE(app.state() == RfChaosApp::State::Done);
  FaradayMode::reset_for_test();
}

void test_rfchaos_fires_targets_round_robin_with_seed() {
  Fixture f;
  FakeFs fs;
  FakeStorage st; st.init();
  yui::FakeClock clock;
  FaradayMode::reset_for_test(); FaradayMode::enable(st);
  RfChaosApp app{fs, clock};
  ChaosCounter a, b;
  app.add_target({"a", std::ref(a), 0});
  app.add_target({"b", std::ref(b), 0});
  app.on_enter(f.hal);
  app.set_intensity(5);
  app.set_seed(42);
  app.on_key(press_fn(Key::Enter));
  for (uint32_t t = 0; t < 5000; t += 100) app.tick(t);
  TEST_ASSERT_TRUE(a.n + b.n >= 5);
  FaradayMode::reset_for_test();
}

void test_rfchaos_writes_log_on_stop() {
  Fixture f;
  FakeFs fs;
  FakeStorage st; st.init();
  yui::FakeClock clock;
  FaradayMode::reset_for_test(); FaradayMode::enable(st);
  RfChaosApp app{fs, clock};
  ChaosCounter c;
  app.add_target({"a", std::ref(c), 0});
  app.on_enter(f.hal);
  app.set_intensity(10);
  app.set_seed(1);
  app.on_key(press_fn(Key::Enter));
  for (uint32_t t = 0; t < 500; t += 50) app.tick(t);
  app.on_key(press(Key::Backspace));  // manual stop → flush log
  yui::FsEntry entries[8];
  int n = fs.list("/rfchaos", entries, 8);
  TEST_ASSERT_TRUE(n >= 1);
  FaradayMode::reset_for_test();
}

void test_rfchaos_intensity_clamps() {
  FakeFs fs;
  yui::FakeClock clock;
  RfChaosApp app{fs, clock};
  app.set_intensity(0);
  TEST_ASSERT_EQUAL_INT(1, app.intensity());
  app.set_intensity(100);
  TEST_ASSERT_EQUAL_INT(10, app.intensity());
}

void test_rfchaos_duration_minutes() {
  FakeFs fs;
  yui::FakeClock clock;
  RfChaosApp app{fs, clock};
  app.set_duration(RfChaosApp::Duration::OneMin);     TEST_ASSERT_EQUAL_INT(1,  app.duration_minutes());
  app.set_duration(RfChaosApp::Duration::FiveMin);    TEST_ASSERT_EQUAL_INT(5,  app.duration_minutes());
  app.set_duration(RfChaosApp::Duration::FifteenMin); TEST_ASSERT_EQUAL_INT(15, app.duration_minutes());
  app.set_duration(RfChaosApp::Duration::SixtyMin);   TEST_ASSERT_EQUAL_INT(60, app.duration_minutes());
}

// ───── Phase 5.2 — Dual-rail BLE TX (HAL only; app integration TBD) ─────

void test_ble_rawtx_nimble_rail_accepts_only_adv_channels() {
  yui::FakeNimBleRawTx rail;
  TEST_ASSERT_TRUE(rail.set_channel(37));
  TEST_ASSERT_TRUE(rail.set_channel(38));
  TEST_ASSERT_TRUE(rail.set_channel(39));
  TEST_ASSERT_FALSE(rail.set_channel(36));
  TEST_ASSERT_FALSE(rail.set_channel(40));
  TEST_ASSERT_FALSE(rail.set_channel(0));
}

void test_ble_rawtx_nimble_rail_records_payload_and_count() {
  yui::FakeNimBleRawTx rail;
  rail.set_channel(37);
  uint8_t p[] = {0x02, 0x01, 0x06};   // typical "flags" adv byte trio
  TEST_ASSERT_TRUE(rail.tx_advert(p, sizeof(p)));
  TEST_ASSERT_TRUE(rail.tx_advert(p, sizeof(p)));
  TEST_ASSERT_EQUAL_size_t(2, rail.tx_count());
  TEST_ASSERT_EQUAL_size_t(3, rail.last_payload().size());
}

void test_ble_rawtx_nrf24_channel_mapping_is_correct() {
  // BLE adv channel ↔ nRF24 channel (channel = MHz - 2400):
  //   37 → 2402 → nRF24 ch 2
  //   38 → 2426 → nRF24 ch 26
  //   39 → 2480 → nRF24 ch 80
  using Rail = yui::FakeNrf24BleRawTx;
  TEST_ASSERT_EQUAL_INT(2,  Rail::adv_channel_to_nrf24_channel(37));
  TEST_ASSERT_EQUAL_INT(26, Rail::adv_channel_to_nrf24_channel(38));
  TEST_ASSERT_EQUAL_INT(80, Rail::adv_channel_to_nrf24_channel(39));
  TEST_ASSERT_EQUAL_INT(-1, Rail::adv_channel_to_nrf24_channel(0));
  Rail rail;
  rail.set_channel(38);
  TEST_ASSERT_EQUAL_INT(26, rail.nrf_channel());
}

void test_ble_rawtx_nrf24_rail_rejects_invalid_channel() {
  yui::FakeNrf24BleRawTx rail;
  TEST_ASSERT_TRUE(rail.set_channel(37));
  TEST_ASSERT_FALSE(rail.set_channel(36));
  TEST_ASSERT_FALSE(rail.set_channel(40));
}

void test_ble_rawtx_continuous_start_stop_lifecycle() {
  yui::FakeNrf24BleRawTx rail;
  TEST_ASSERT_FALSE(rail.is_active());
  TEST_ASSERT_TRUE(rail.start_continuous(38));
  TEST_ASSERT_TRUE(rail.is_active());
  TEST_ASSERT_EQUAL_INT(38, rail.channel());
  rail.stop();
  TEST_ASSERT_FALSE(rail.is_active());
}

void test_ble_rawtx_dual_rail_independent_counters() {
  yui::FakeNimBleRawTx nimble;
  yui::FakeNrf24BleRawTx nrf24;
  nimble.set_channel(37);
  nrf24.set_channel(38);
  uint8_t p[] = {0x01};
  nimble.tx_advert(p, 1);
  nrf24.tx_advert(p, 1);
  nrf24.tx_advert(p, 1);
  TEST_ASSERT_EQUAL_size_t(1, nimble.tx_count());
  TEST_ASSERT_EQUAL_size_t(2, nrf24.tx_count());
}

void test_ble_rawtx_payload_over_31_bytes_is_rejected() {
  yui::FakeNimBleRawTx rail;
  uint8_t big[40] = {0};
  TEST_ASSERT_FALSE(rail.tx_advert(big, sizeof(big)));
  TEST_ASSERT_EQUAL_size_t(0, rail.tx_count());
}

void test_ble_rawtx_cap_missing_blocks_tx() {
  yui::FakeNrf24BleRawTx rail;
  rail.set_present(false);
  uint8_t p[] = {0x01};
  TEST_ASSERT_FALSE(rail.tx_advert(p, 1));
  TEST_ASSERT_EQUAL_size_t(0, rail.tx_count());
}

void test_ble_rawtx_channel_split_no_overlap() {
  // The default Phase 5.2 channel split (NimBLE→37, nRF24→38+39) leaves
  // exactly one rail per channel — no two rails on the same channel.
  yui::FakeNimBleRawTx nimble;
  yui::FakeNrf24BleRawTx nrf24;
  nimble.set_channel(37);
  nrf24.set_channel(38);
  TEST_ASSERT_NOT_EQUAL(nimble.channel(), nrf24.channel());
  nrf24.set_channel(39);
  TEST_ASSERT_NOT_EQUAL(nimble.channel(), nrf24.channel());
}

void test_ble_rawtx_rail_names() {
  yui::FakeNimBleRawTx nimble;
  yui::FakeNrf24BleRawTx nrf24;
  TEST_ASSERT_EQUAL_STRING("NimBLE", nimble.rail_name());
  TEST_ASSERT_EQUAL_STRING("nRF24",  nrf24.rail_name());
}

void test_settings_faraday_row_toggles() {
  Fixture f;
  FakeStorage store; store.init();
  FaradayMode::reset_for_test();
  SettingsApp app{store};
  app.on_enter(f.hal);
  app.set_cursor(4);  // Faraday Mode row
  TEST_ASSERT_FALSE(FaradayMode::is_active());
  app.on_key(press(Key::Right));
  TEST_ASSERT_TRUE(FaradayMode::is_active());
  app.on_key(press(Key::Left));
  TEST_ASSERT_FALSE(FaradayMode::is_active());
  // Cleanup so other tests start with a clean flag.
  FaradayMode::reset_for_test();
}

// ───── KoiGotchiApp ────────────────────────────────────────────────────────

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
  PineappleApp app{h, st};
  app.on_enter(f.hal);
  app.set_tab(PineappleApp::Tab::Handshakes);
  TEST_ASSERT_TRUE(app.handshakes_ok());
  TEST_ASSERT_EQUAL_size_t(2u, app.count());
}

// RogueApApp — EvilTwin mode

namespace {

template <class Make>
void rogue_with(Make make) {
  // helper kept simple — tests just construct in-place below
  (void)make;
}

}  // namespace

void test_evil_twin_fn_enter_enables() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  FakeWifiAp ap;
  FakeFs fs;
  seed_pineapple_creds(st);
  register_login_ok(h);
  h.register_response("PUT",  "http://pa.local:1471/api/pineap/settings",
                      200, "{\"success\":true}");
  RogueApApp app{h, st, ap, fs};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.mode() == RogueApApp::Mode::EvilTwin);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(app.eviltwin_enabled());
  TEST_ASSERT_TRUE(app.eviltwin_last_ok());
}

void test_evil_twin_plain_enter_does_nothing() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  FakeWifiAp ap;
  FakeFs fs;
  seed_pineapple_creds(st);
  register_login_ok(h);
  RogueApApp app{h, st, ap, fs};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_FALSE(app.eviltwin_enabled());
}

// RogueApApp — Karma mode

void test_karma_fn_enter_enables() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  seed_pineapple_creds(st);
  register_login_ok(h);
  h.register_response("PUT",  "http://pa.local:1471/api/pineap/settings",
                      200, "{\"success\":true}");
  FakeWifiAp ap;
  FakeFs fs;
  RogueApApp app{h, st, ap, fs};
  app.on_enter(f.hal);
  app.set_mode(RogueApApp::Mode::Karma);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(app.karma_enabled());
}

// DeauthApp — Pineapple backend

void test_deauth_armed_required_to_fire() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  FakeWifiMonitor mon;
  seed_pineapple_creds(st);
  register_login_ok(h);
  h.register_response("POST", "http://pa.local:1471/api/pineap/deauth/ap",
                      200, "{\"success\":true}");
  DeauthApp app{h, st, mon};
  TEST_ASSERT_TRUE(app.backend() == DeauthApp::Backend::Pineapple);
  app.on_enter(f.hal);
  app.set_target("AA:BB:CC:DD:EE:FF", uint8_t{6});
  app.on_key(press_fn(Key::Enter));   // not armed → no fire
  TEST_ASSERT_FALSE(app.fired());
  app.on_key(press(Key::Tab));        // arm
  TEST_ASSERT_TRUE(app.armed());
  app.on_key(press_fn(Key::Enter));   // fire
  TEST_ASSERT_TRUE(app.fired());
  TEST_ASSERT_TRUE(app.last_ok());
}

void test_deauth_b_key_toggles_backend() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  FakeWifiMonitor mon;
  DeauthApp app{h, st, mon};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.backend() == DeauthApp::Backend::Pineapple);
  KeyEvent kb{}; kb.key = Key::Char; kb.ch = 'b'; kb.down = true;
  app.on_key(kb);
  TEST_ASSERT_TRUE(app.backend() == DeauthApp::Backend::Native);
  app.on_key(kb);
  TEST_ASSERT_TRUE(app.backend() == DeauthApp::Backend::Pineapple);
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

// DeauthApp — Native backend

void test_native_deauth_disarmed_by_default() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  FakeWifiMonitor mon;
  DeauthApp app{h, st, mon};
  app.set_backend(DeauthApp::Backend::Native);
  app.on_enter(f.hal);
  app.set_target("AA:BB:CC:DD:EE:FF", "11:22:33:44:55:66");
  app.on_key(press_fn(Key::Enter));   // Fn+Enter without Tab → no fire
  app.tick(0);
  TEST_ASSERT_FALSE(app.firing());
  TEST_ASSERT_EQUAL_size_t(0u, app.tx_total());
}

void test_native_deauth_arm_and_fire_emits_frames() {
  Fixture f;
  FakeHttp h;
  FakeStorage st;
  FakeWifiMonitor mon;
  DeauthApp app{h, st, mon};
  app.set_backend(DeauthApp::Backend::Native);
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
  FakeHttp h;
  FakeStorage st;
  FakeWifiMonitor mon;
  DeauthApp app{h, st, mon};
  app.set_backend(DeauthApp::Backend::Native);
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

// RogueApApp — Captive mode

void test_captive_portal_idle_until_started() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeHttp h;
  FakeStorage st;
  RogueApApp app{h, st, ap, fs};
  app.on_enter(f.hal);
  app.set_mode(RogueApApp::Mode::Captive);
  TEST_ASSERT_TRUE(app.state() == RogueApApp::CaptiveState::Idle);
  TEST_ASSERT_FALSE(ap.active());
}

void test_captive_portal_fn_enter_starts_ap() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeHttp h;
  FakeStorage st;
  st.put_str("cp.ssid", "FreeWiFi");
  RogueApApp app{h, st, ap, fs};
  app.on_enter(f.hal);
  app.set_mode(RogueApApp::Mode::Captive);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == RogueApApp::CaptiveState::Running);
  TEST_ASSERT_TRUE(ap.active());
  TEST_ASSERT_TRUE(ap.captive());
}

void test_captive_portal_no_ssid_does_not_start() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeHttp h;
  FakeStorage st;
  RogueApApp app{h, st, ap, fs};
  app.on_enter(f.hal);
  app.set_mode(RogueApApp::Mode::Captive);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_FALSE(ap.active());
}

void test_captive_portal_captures_form_submissions() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeHttp h;
  FakeStorage st;
  st.put_str("cp.ssid", "X");
  RogueApApp app{h, st, ap, fs};
  app.on_enter(f.hal);
  app.set_mode(RogueApApp::Mode::Captive);
  app.on_key(press_fn(Key::Enter));
  ap.simulate_form_submit("192.168.4.2", "u=alice&p=hunter2");
  ap.simulate_form_submit("192.168.4.3", "u=bob&p=qwerty");
  TEST_ASSERT_EQUAL_size_t(2u, app.capture_count());
  TEST_ASSERT_EQUAL_STRING("u=alice&p=hunter2", app.capture_at(0).body);
  TEST_ASSERT_EQUAL_STRING("192.168.4.3", app.capture_at(1).peer_ip);
}

void test_captive_portal_fn_tab_writes_captures_to_sd() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeHttp h;
  FakeStorage st;
  st.put_str("cp.ssid", "X");
  RogueApApp app{h, st, ap, fs};
  app.on_enter(f.hal);
  app.set_mode(RogueApApp::Mode::Captive);
  app.on_key(press_fn(Key::Enter));
  ap.simulate_form_submit("10.0.0.1", "u=test");
  app.on_key(press_fn(Key::Tab));
  TEST_ASSERT_TRUE(fs.exists("/captures.txt"));
}

void test_rogue_ap_tab_cycles_three_modes() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeHttp h;
  FakeStorage st;
  RogueApApp app{h, st, ap, fs};
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.mode() == RogueApApp::Mode::EvilTwin);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.mode() == RogueApApp::Mode::Karma);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.mode() == RogueApApp::Mode::Captive);
  app.on_key(press(Key::Tab));
  TEST_ASSERT_TRUE(app.mode() == RogueApApp::Mode::EvilTwin);
}

void test_rogue_ap_switching_mode_stops_captive() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeHttp h;
  FakeStorage st;
  st.put_str("cp.ssid", "X");
  RogueApApp app{h, st, ap, fs};
  app.on_enter(f.hal);
  app.set_mode(RogueApApp::Mode::Captive);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(ap.active());
  app.set_mode(RogueApApp::Mode::EvilTwin);
  TEST_ASSERT_FALSE(ap.active());
}

// ───── SatTracker — TLE parser ────────────────────────────────────────────

void test_tle_parses_iss_sample() {
  // A representative ISS TLE (epoch 2024 day 1.5).
  const char* l1 =
    "1 25544U 98067A   24001.50000000  .00012345  00000-0  22345-3 0  9999";
  const char* l2 =
    "2 25544  51.6400 123.4567 0001234 234.5678 125.4321 15.50000000123456";
  sat::TleElements el;
  TEST_ASSERT_TRUE(sat::parse_tle(l1, l2, el));
  TEST_ASSERT_EQUAL_UINT32(25544u, el.norad_id);
  TEST_ASSERT_EQUAL_INT('U', el.classification);
  TEST_ASSERT_EQUAL_STRING("98067A", el.intl_designator);
  TEST_ASSERT_EQUAL_INT(2024, el.epoch_year);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.5, el.epoch_day);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 51.6400, el.inclination_deg);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0001234, el.eccentricity);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 15.50000000, el.mean_motion);
}

void test_tle_compressed_exp_decode() {
  bool ok = false;
  // " 12345-3" → 0.12345e-3 = 0.00012345
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.00012345,
      sat::tle_detail::parse_compressed(" 12345-3", ok));
  TEST_ASSERT_TRUE(ok);
  // "-12345+0" → -0.12345
  ok = false;
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, -0.12345,
      sat::tle_detail::parse_compressed("-12345+0", ok));
  TEST_ASSERT_TRUE(ok);
  // " 00000+0" → 0
  ok = false;
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0,
      sat::tle_detail::parse_compressed(" 00000+0", ok));
  TEST_ASSERT_TRUE(ok);
}

void test_tle_rejects_mismatched_catalog() {
  const char* l1 =
    "1 25544U 98067A   24001.50000000  .00012345  00000-0  22345-3 0  9999";
  const char* l2 =
    "2 25555  51.6400 123.4567 0001234 234.5678 125.4321 15.50000000123456";
  sat::TleElements el;
  TEST_ASSERT_FALSE(sat::parse_tle(l1, l2, el));
}

void test_tle_year_window() {
  const char* l1 =
    "1 25544U 98067A   97001.50000000  .00012345  00000-0  22345-3 0  9999";
  const char* l2 =
    "2 25544  51.6400 123.4567 0001234 234.5678 125.4321 15.50000000123456";
  sat::TleElements el;
  TEST_ASSERT_TRUE(sat::parse_tle(l1, l2, el));
  TEST_ASSERT_EQUAL_INT(1997, el.epoch_year);
}

// ───── SatTracker — Vec3 math ─────────────────────────────────────────────

void test_vec3_basic_ops() {
  sat::Vec3 a{1, 2, 3}, b{4, 5, 6};
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 32.0, a.dot(b));
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, std::sqrt(14.0), a.mag());
  sat::Vec3 c = a + b;
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 5.0, c.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 9.0, c.z);
}

void test_vec3_normalized_unit() {
  sat::Vec3 v{3, 4, 0};
  sat::Vec3 n = v.normalized();
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, n.mag());
}

// ───── SatTracker — time helpers ──────────────────────────────────────────

void test_jd_from_unix_epoch_match() {
  // 1970-01-01 00:00:00 UTC = JD 2440587.5
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 2440587.5, sat::jd_from_unix(0));
}

void test_gstime_known_value() {
  // GMST at J2000 should be ~280.46° (= 4.894... rad).
  // Vallado example, very rough check.
  const double gmst = sat::gstime(sat::kJ2000);
  // 280.4606° in rad
  TEST_ASSERT_DOUBLE_WITHIN(0.05, 4.8949612, gmst);
}

// ───── SatTracker — Propagator ────────────────────────────────────────────

namespace {
const char* iss_tle_l1 =
  "1 25544U 98067A   24001.50000000  .00012345  00000-0  22345-3 0  9999";
const char* iss_tle_l2 =
  "2 25544  51.6400 123.4567 0001234 234.5678 125.4321 15.50000000123456";
}

void test_propagator_init_succeeds() {
  sat::TleElements el;
  sat::parse_tle(iss_tle_l1, iss_tle_l2, el);
  sat::Propagator p;
  TEST_ASSERT_TRUE(p.init(el));
  TEST_ASSERT_TRUE(p.inited());
}

void test_propagator_position_at_epoch_is_finite_and_leo() {
  sat::TleElements el;
  sat::parse_tle(iss_tle_l1, iss_tle_l2, el);
  sat::Propagator p;
  p.init(el);
  sat::StateVector sv;
  TEST_ASSERT_TRUE(p.propagate(0.0, sv));
  // ISS altitude ~420 km → distance from Earth center ~6798 km
  const double r = sv.position.mag();
  TEST_ASSERT_TRUE(r > 6700.0 && r < 6900.0);
  // Velocity ~7.66 km/s for LEO
  const double v = sv.velocity.mag();
  TEST_ASSERT_TRUE(v > 7.0 && v < 8.5);
}

void test_propagator_position_changes_with_time() {
  sat::TleElements el;
  sat::parse_tle(iss_tle_l1, iss_tle_l2, el);
  sat::Propagator p;
  p.init(el);
  sat::StateVector a, b;
  p.propagate(0.0,    a);
  p.propagate(60.0,   b);   // one minute later
  // Distance moved should be roughly v * 60s ~= 460 km
  const double moved = (b.position - a.position).mag();
  TEST_ASSERT_TRUE(moved > 200.0 && moved < 700.0);
}

void test_propagator_one_orbit_returns_close_to_start() {
  // After one full orbit (period ~92 min for ISS), position should be
  // roughly back where it started — within tens of km given J2 secular
  // drift (only Ω/ω/M precess, not the position itself in absolute terms).
  sat::TleElements el;
  sat::parse_tle(iss_tle_l1, iss_tle_l2, el);
  sat::Propagator p;
  p.init(el);
  sat::StateVector a, b;
  p.propagate(0.0, a);
  // mean motion 15.5 rev/day → period = 86400 / 15.5 sec
  const double T = 86400.0 / 15.5;
  p.propagate(T, b);
  const double err = (b.position - a.position).mag();
  TEST_ASSERT_TRUE(err < 200.0);
}

// ───── SatTracker — topocentric look angles ───────────────────────────────

void test_topo_satellite_overhead_is_90deg_elevation() {
  // Pick any JD; place a satellite directly overhead an observer at
  // (0°N, 0°E) by computing the corresponding ECI vector from ECEF.
  // Observer ECEF at (0,0,0)° = (aE, 0, 0). Apply ECEF→ECI = inverse
  // of eci_to_ecef (rotate +z by gmst).
  const double jd = sat::kJ2000 + 1.234;
  const double g  = sat::gstime(jd);
  const double r  = sat::kEarthRadius_km + 400.0;   // 400 km up
  // ECEF (r, 0, 0) → ECI:
  sat::StateVector sv;
  sv.position = {r * std::cos(g), r * std::sin(g), 0.0};
  sv.velocity = {0, 1, 0};
  sat::ObserverGeodetic obs{0.0, 0.0, 0.0};
  sat::LookAngles la = sat::look_angles(sv, obs, jd);
  TEST_ASSERT_TRUE(la.elevation_deg > 85.0);
}

void test_topo_doppler_sign_convention() {
  // Receding (vr > 0) → red shift → negative Doppler
  TEST_ASSERT_TRUE(sat::doppler_hz(437.8e6, +1.0) < 0);
  TEST_ASSERT_TRUE(sat::doppler_hz(437.8e6, -1.0) > 0);
  // Magnitude: |Δf| = f * |vr| / c
  // 1 km/s on 437.8 MHz → ~1.46 kHz
  const double df = sat::doppler_hz(437.8e6, -1.0);
  TEST_ASSERT_DOUBLE_WITHIN(50.0, 1460.0, df);
}

// ───── SatTracker — pass predictor ────────────────────────────────────────

// ───── SatTrackerApp ──────────────────────────────────────────────────────

void test_sattracker_loads_default_tles_when_no_sd() {
  Fixture f;
  FakeRadioLink r;
  FakeGnss g;
  FakeFs fs;   // empty
  SatTrackerApp app{r, g, fs, f.clock};
  app.on_enter(f.hal);
  // Defaults include ISS + SO-50
  TEST_ASSERT_TRUE(app.favorite_count() >= 2);
  TEST_ASSERT_TRUE(app.view() == SatTrackerApp::View::List);
}

void test_sattracker_loads_tles_from_sd_if_present() {
  Fixture f;
  FakeRadioLink r;
  FakeGnss g;
  FakeFs fs;
  const char* records =
    "ISS\n"
    "1 25544U 98067A   24001.50000000  .00012345  00000-0  22345-3 0  9999\n"
    "2 25544  51.6400 123.4567 0001234 234.5678 125.4321 15.50000000123456\n";
  fs.write_all("/yui-tles.txt", records, std::strlen(records));
  SatTrackerApp app{r, g, fs, f.clock};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(1u, app.favorite_count());
  TEST_ASSERT_EQUAL_STRING("ISS", app.favorite_at(0).name);
}

void test_sattracker_arrow_keys_move_cursor_in_list() {
  Fixture f;
  FakeRadioLink r;
  FakeGnss g;
  FakeFs fs;
  SatTrackerApp app{r, g, fs, f.clock};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(0u, app.cursor());
  app.on_key(press(Key::Down));
  TEST_ASSERT_EQUAL_size_t(1u, app.cursor());
  app.on_key(press(Key::Up));
  TEST_ASSERT_EQUAL_size_t(0u, app.cursor());
}

void test_sattracker_enter_drills_into_detail() {
  Fixture f;
  FakeRadioLink r;
  FakeGnss g;
  FakeFs fs;
  SatTrackerApp app{r, g, fs, f.clock};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.view() == SatTrackerApp::View::Detail);
}

void test_sattracker_tab_in_detail_starts_live() {
  Fixture f;
  FakeRadioLink r;
  FakeGnss g;
  FakeFs fs;
  SatTrackerApp app{r, g, fs, f.clock};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));   // → Detail
  app.on_key(press(Key::Tab));     // → Live
  TEST_ASSERT_TRUE(app.view() == SatTrackerApp::View::Live);
}

void test_sattracker_live_writes_doppler_to_radio() {
  Fixture f;
  FakeRadioLink r;
  r.connect("MAC", "0000");
  r.register_reply("FQ 0,0145824998",  "FQ 0,0145824998");   // any reply OK
  // We don't know the exact freq the propagator will compute, so register
  // a wildcard via several common shifts. Easier: register the bare prefix.
  // But FakeRadioLink does exact-match. Use a generic catchall: register
  // the typical iss freq with no shift, plus a few variants. The test
  // only asserts that *some* command succeeded, so register any matching
  // command we expect. Robust path: register the exact computed freq by
  // pre-running the propagator here:
  FakeGnss g;
  GnssFix fx; fx.valid = true;
  fx.lat_deg = 49; fx.lon_deg = -72; fx.altitude_m = 0;
  fx.epoch_seconds = 1735689600ULL;
  g.set_fix(fx);
  FakeFs fs;
  SatTrackerApp app{r, g, fs, f.clock};
  app.on_enter(f.hal);
  app.on_key(press(Key::Enter));   // Detail
  app.on_key(press(Key::Tab));     // Live
  // Register a catchall for whatever FQ command gets sent. Walk the
  // possible Doppler-shifted freqs near 145.825 MHz ±5 kHz and seed.
  for (long long base = 145820000LL; base <= 145830000LL; ++base) {
    char cmd[32], reply[40];
    std::snprintf(cmd, sizeof(cmd), "FQ 0,%010lld", base);
    std::snprintf(reply, sizeof(reply), "%s", cmd);
    r.register_reply(cmd, reply);
  }
  app.tick(0);    // first call past kDopplerIntervalMs threshold
  app.tick(3000); // past threshold → second push
  TEST_ASSERT_TRUE(app.doppler_writes() >= 1);
}

void test_pass_predictor_finds_a_pass_within_24h() {
  sat::TleElements el;
  sat::parse_tle(iss_tle_l1, iss_tle_l2, el);
  sat::Propagator p;
  p.init(el);
  // Observer at 49°N (similar to test TLE epoch's likely visibility window)
  sat::ObserverGeodetic obs{49.0, -72.0, 0};
  const double jd_epoch = sat::jd_from_year_day(el.epoch_year, el.epoch_day);
  sat::PassInfo info = sat::predict_next_pass(p, obs, jd_epoch, 24.0, 0.0, 30.0);
  TEST_ASSERT_TRUE(info.found);
  TEST_ASSERT_TRUE(info.aos_jd > jd_epoch);
  TEST_ASSERT_TRUE(info.los_jd > info.aos_jd);
  TEST_ASSERT_TRUE(info.max_elevation_deg >= 0.0);
}

// ───── v0.3 review fixes — regression coverage ────────────────────────────

void test_pass_predictor_starting_mid_pass_pins_aos_to_search_start() {
  // Regression for Pass.hpp aos_t = -1 leak: if the predictor starts
  // search mid-pass, an LOS within the window must produce a PassInfo
  // whose aos_jd is the search start (not garbage from -1 sentinel).
  sat::TleElements el;
  sat::parse_tle(iss_tle_l1, iss_tle_l2, el);
  sat::Propagator p;
  p.init(el);
  sat::ObserverGeodetic obs{49.0, -72.0, 0};
  const double jd_epoch = sat::jd_from_year_day(el.epoch_year, el.epoch_day);
  // First find any AOS/LOS so we know a real pass window
  sat::PassInfo full = sat::predict_next_pass(p, obs, jd_epoch, 24.0, 0.0, 30.0);
  TEST_ASSERT_TRUE(full.found);
  // Now restart search from halfway between AOS and LOS — i.e. mid-pass.
  const double jd_mid = 0.5 * (full.aos_jd + full.los_jd);
  sat::PassInfo mid = sat::predict_next_pass(p, obs, jd_mid, 24.0, 0.0, 30.0);
  TEST_ASSERT_TRUE(mid.found);
  // aos_jd must be ≥ jd_mid (the search start) — never the -1 sentinel
  // which would fall well before jd_epoch.
  TEST_ASSERT_TRUE(mid.aos_jd >= jd_mid - 1e-6);
  TEST_ASSERT_TRUE(mid.aos_jd > jd_epoch);
  TEST_ASSERT_TRUE(mid.los_jd > mid.aos_jd);
}

void test_sattracker_accepts_two_line_tle_no_name() {
  // Regression for SatTrackerApp::add_tle_record_ 2-line shift: a TLE
  // file with only L1+L2 (no name line) must parse, not be silently
  // discarded.
  Fixture f;
  FakeRadioLink r;
  FakeGnss g;
  FakeFs fs;
  const char* records =
    "1 25544U 98067A   24001.50000000  .00012345  00000-0  22345-3 0  9999\n"
    "2 25544  51.6400 123.4567 0001234 234.5678 125.4321 15.50000000123456\n";
  fs.write_all("/yui-tles.txt", records, std::strlen(records));
  SatTrackerApp app{r, g, fs, f.clock};
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_size_t(1u, app.favorite_count());
  TEST_ASSERT_EQUAL_STRING("(unnamed)", app.favorite_at(0).name);
}

// ───── v0.3 gap fills — sat module ────────────────────────────────────────

void test_vec3_mag_and_mag2() {
  // 3-4-5 right triangle: |(3,4,0)| = 5, mag2 = 25.
  const sat::Vec3 v{3.0, 4.0, 0.0};
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 25.0, v.mag2());
  TEST_ASSERT_DOUBLE_WITHIN(1e-12,  5.0, v.mag());
  // Negative components: magnitude is unsigned.
  const sat::Vec3 w{-1.0, -2.0, -2.0};
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 9.0, w.mag2());
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 3.0, w.mag());
  // Zero vector.
  const sat::Vec3 z{0, 0, 0};
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, z.mag());
}

void test_jd_from_unix_round_trip_to_j2000() {
  // Unix epoch 1970-01-01 00:00 UTC = JD 2440587.5
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 2440587.5, sat::jd_from_unix(0.0));
  // 2000-01-01 12:00 UTC (= kJ2000) in unix seconds = 946728000
  const double jd_noon = sat::jd_from_unix(946728000.0);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, sat::kJ2000, jd_noon);
  // 2000-01-01 00:00 UTC = JD 2451544.5 (kJ2000 - 0.5)
  const double jd_mid = sat::jd_from_unix(946684800.0);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, sat::kJ2000 - 0.5, jd_mid);
}

void test_topo_observer_ecef_at_known_points() {
  // (0°N, 0°E, 0m) → (R, 0, 0)
  const sat::Vec3 a = sat::observer_ecef_km({0, 0, 0});
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, sat::kEarthRadius_km, a.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, a.y);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, a.z);
  // North pole: (90°N, 0°E, 0m) → (0, 0, R)
  const sat::Vec3 n = sat::observer_ecef_km({90, 0, 0});
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, n.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, n.y);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, sat::kEarthRadius_km, n.z);
  // Altitude adds to radius.
  const sat::Vec3 up = sat::observer_ecef_km({0, 0, 1000.0});  // 1 km up
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, sat::kEarthRadius_km + 1.0, up.x);
}

void test_topo_eci_to_ecef_zero_gmst_is_identity() {
  const sat::Vec3 v{1.0, 2.0, 3.0};
  const sat::Vec3 r = sat::eci_to_ecef(v, 0.0);
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 1.0, r.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 2.0, r.y);
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 3.0, r.z);
  // Quarter-turn (gmst = π/2): (1,0,0) → (0,-1,0); z preserved.
  const sat::Vec3 q = sat::eci_to_ecef({1.0, 0.0, 5.0}, M_PI / 2.0);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9,  0.0, q.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, -1.0, q.y);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9,  5.0, q.z);
}

// ───── v0.3 gap fills — fake HAL fixtures ─────────────────────────────────

void test_fake_ble_central_connect_enumerate_read() {
  FakeBleCentral c;
  // Register a synthetic battery service.
  std::vector<FakeBleCentral::CharSpec> chars = {
    {"00002a19-0000-1000-8000-00805f9b34fb", 0x12, {77}},   // battery level: 77%
  };
  c.register_service("AA:BB:CC:DD:EE:FF",
                     "0000180f-0000-1000-8000-00805f9b34fb", chars);
  // Connecting to an unknown MAC fails; known MAC succeeds.
  TEST_ASSERT_FALSE(c.connect("11:22:33:44:55:66"));
  TEST_ASSERT_TRUE(c.connect("AA:BB:CC:DD:EE:FF"));
  TEST_ASSERT_TRUE(c.connected());
  // Service enumeration.
  GattService svcs[4];
  const size_t ns = c.enumerate_services(svcs, 4);
  TEST_ASSERT_EQUAL_size_t(1u, ns);
  TEST_ASSERT_EQUAL_STRING("0000180f-0000-1000-8000-00805f9b34fb", svcs[0].uuid);
  // Char enumeration.
  GattCharacteristic chs[4];
  const size_t nc = c.enumerate_characteristics(svcs[0].uuid, chs, 4);
  TEST_ASSERT_EQUAL_size_t(1u, nc);
  TEST_ASSERT_EQUAL_HEX8(0x12, chs[0].properties);
  // Read characteristic value.
  uint8_t buf[4] = {0};
  const int rd = c.read_characteristic(svcs[0].uuid, chs[0].uuid, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(1, rd);
  TEST_ASSERT_EQUAL_UINT8(77, buf[0]);
  // Disconnect → enumeration returns 0, reads return -1.
  c.disconnect();
  TEST_ASSERT_FALSE(c.connected());
  TEST_ASSERT_EQUAL_size_t(0u, c.enumerate_services(svcs, 4));
  TEST_ASSERT_EQUAL_INT(-1, c.read_characteristic(svcs[0].uuid, chs[0].uuid,
                                                  buf, sizeof(buf)));
}

namespace {
struct WifiTapCtx {
  size_t calls = 0;
  WifiPktType last_type = WifiPktType::Misc;
  size_t last_len = 0;
};
void wifi_tap_cb(void* ctx, const uint8_t* /*frame*/, size_t len,
                 const WifiRxMeta& meta) {
  auto* w = static_cast<WifiTapCtx*>(ctx);
  ++w->calls;
  w->last_type = meta.type;
  w->last_len  = len;
}
}  // namespace

void test_fake_wifi_monitor_filters_by_pkt_type() {
  FakeWifiMonitor m;
  WifiTapCtx tap;
  m.set_callback(wifi_tap_cb, &tap);
  WifiFilter filt;
  filt.mgmt = true; filt.ctrl = false; filt.data = false; filt.misc = false;
  TEST_ASSERT_TRUE(m.start(filt, 6));
  TEST_ASSERT_TRUE(m.running());
  TEST_ASSERT_EQUAL_UINT8(6, m.channel());
  // Mgmt frame is delivered.
  uint8_t fr[8] = {0x80, 0x00};
  WifiRxMeta meta_mgmt{-50, 6, WifiPktType::Management};
  m.inject_frame(fr, sizeof(fr), meta_mgmt);
  TEST_ASSERT_EQUAL_size_t(1u, tap.calls);
  TEST_ASSERT_EQUAL_size_t(8u, tap.last_len);
  // Data frame is filtered out (filt.data = false).
  WifiRxMeta meta_data{-60, 6, WifiPktType::Data};
  m.inject_frame(fr, sizeof(fr), meta_data);
  TEST_ASSERT_EQUAL_size_t(1u, tap.calls);   // unchanged
  // Channel hop.
  TEST_ASSERT_TRUE(m.set_channel(11));
  TEST_ASSERT_EQUAL_UINT8(11, m.channel());
  // tx_raw logs frames while running.
  uint8_t tx[3] = {0xAA, 0xBB, 0xCC};
  TEST_ASSERT_TRUE(m.tx_raw(tx, sizeof(tx)));
  TEST_ASSERT_EQUAL_size_t(1u, m.tx_count());
  // Stop → tx fails, frames dropped.
  m.stop();
  TEST_ASSERT_FALSE(m.running());
  TEST_ASSERT_FALSE(m.tx_raw(tx, sizeof(tx)));
  m.inject_frame(fr, sizeof(fr), meta_mgmt);
  TEST_ASSERT_EQUAL_size_t(1u, tap.calls);   // still unchanged
}

void test_captive_portal_backspace_stops_ap() {
  Fixture f;
  FakeWifiAp ap;
  FakeFs fs;
  FakeHttp h;
  FakeStorage st;
  st.put_str("cp.ssid", "X");
  RogueApApp app{h, st, ap, fs};
  app.on_enter(f.hal);
  app.set_mode(RogueApApp::Mode::Captive);
  app.on_key(press_fn(Key::Enter));
  TEST_ASSERT_TRUE(ap.active());
  app.on_key(press(Key::Backspace));
  TEST_ASSERT_FALSE(ap.active());
  TEST_ASSERT_TRUE(app.state() == RogueApApp::CaptiveState::Stopped);
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
  PineappleApp app{h, st};
  app.on_enter(f.hal);
  app.set_tab(PineappleApp::Tab::Recon);
  app.on_key(press(Key::Enter));
  app.tick(0); app.tick(2000);
  TEST_ASSERT_TRUE(app.state() == PineappleApp::ReconState::Done);
  app.on_key(press(Key::Enter));
  TEST_ASSERT_TRUE(app.state() == PineappleApp::ReconState::Scanning);
}

// ───── Phase 3 — Hydra HAL (CC1101 + nRF24) + .sub format ──────────────────
#include "yui/hal/ICc1101.hpp"
#include "yui/hal/INrf24.hpp"
#include "../../src/hal/native/NativeCc1101.hpp"
#include "../../src/hal/native/NativeNrf24.hpp"
#include "yui/proto/SubFile.hpp"

void test_native_cc1101_present_by_default_and_can_be_disabled() {
  yui::NativeCc1101 c;
  TEST_ASSERT_TRUE(c.is_present());
  c.set_present(false);
  TEST_ASSERT_FALSE(c.is_present());
  TEST_ASSERT_FALSE(c.begin());
}

void test_native_cc1101_freq_validates_band() {
  yui::NativeCc1101 c;
  TEST_ASSERT_TRUE(c.set_frequency_hz(433'920'000));
  TEST_ASSERT_EQUAL_UINT32(433'920'000, c.frequency_hz());
  TEST_ASSERT_FALSE(c.set_frequency_hz(2'400'000'000));  // out of band
}

void test_native_cc1101_transmit_records_payload() {
  yui::NativeCc1101 c;
  uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
  TEST_ASSERT_TRUE(c.transmit(payload, sizeof(payload)));
  TEST_ASSERT_EQUAL_UINT32(1, c.tx_call_count());
  TEST_ASSERT_EQUAL_UINT64(4, c.tx_bytes());
  TEST_ASSERT_EQUAL_size_t(4, c.last_tx().size());
  TEST_ASSERT_EQUAL_HEX8(0xDE, c.last_tx()[0]);
}

void test_native_cc1101_carrier_toggles() {
  yui::NativeCc1101 c;
  TEST_ASSERT_FALSE(c.carrier_on());
  c.set_carrier(true);
  TEST_ASSERT_TRUE(c.carrier_on());
  c.set_carrier(false);
  TEST_ASSERT_FALSE(c.carrier_on());
}

void test_native_cc1101_receive_drains_queue() {
  yui::NativeCc1101 c;
  c.enqueue_rx({0x01, 0x02, 0x03});
  c.enqueue_rx({0xAA, 0xBB});
  uint8_t buf[16];
  int n = c.receive(buf, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(3, n);
  TEST_ASSERT_EQUAL_HEX8(0x01, buf[0]);
  n = c.receive(buf, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(2, n);
  TEST_ASSERT_EQUAL_HEX8(0xAA, buf[0]);
  n = c.receive(buf, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, n);  // empty
}

void test_native_nrf24_channel_validates() {
  yui::NativeNrf24 r;
  TEST_ASSERT_TRUE(r.set_channel(76));
  TEST_ASSERT_EQUAL_UINT8(76, r.channel());
  TEST_ASSERT_FALSE(r.set_channel(126));  // out of range
}

void test_native_nrf24_address_width_3_to_5() {
  yui::NativeNrf24 r;
  uint8_t a3[3] = {0xAA, 0xBB, 0xCC};
  TEST_ASSERT_TRUE(r.set_address(a3, 3));
  uint8_t a5[5] = {1, 2, 3, 4, 5};
  TEST_ASSERT_TRUE(r.set_address(a5, 5));
  uint8_t a2[2] = {1, 2};
  TEST_ASSERT_FALSE(r.set_address(a2, 2));  // too short
  uint8_t a6[6] = {1, 2, 3, 4, 5, 6};
  TEST_ASSERT_FALSE(r.set_address(a6, 6));  // too long
}

void test_native_nrf24_listen_toggles_state() {
  yui::NativeNrf24 r;
  TEST_ASSERT_FALSE(r.listening());
  r.start_listening();
  TEST_ASSERT_TRUE(r.listening());
  r.stop_listening();
  TEST_ASSERT_FALSE(r.listening());
}

void test_subfile_round_trip_raw_timings() {
  yui::proto::SubHeader h{};
  h.frequency_hz = 433'920'000;
  h.preset = yui::proto::SubPreset::Ook650Async;
  int32_t timings[] = {320, -130, 196, -118, 152, -86};
  std::string text = yui::proto::write_sub(h, timings, 6);

  TEST_ASSERT_TRUE(text.find("Filetype: Flipper SubGhz RAW File") != std::string::npos);
  TEST_ASSERT_TRUE(text.find("Frequency: 433920000") != std::string::npos);
  TEST_ASSERT_TRUE(text.find("Preset: FuriHalSubGhzPresetOok650Async") != std::string::npos);
  TEST_ASSERT_TRUE(text.find(" 320 -130") != std::string::npos);

  yui::proto::SubHeader hp{};
  std::vector<int32_t> back;
  bool ok = yui::proto::read_sub_to_vector(text.data(), text.size(), hp, back);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT32(433'920'000, hp.frequency_hz);
  TEST_ASSERT_TRUE(hp.preset == yui::proto::SubPreset::Ook650Async);
  TEST_ASSERT_EQUAL_size_t(6, back.size());
  TEST_ASSERT_EQUAL_INT32(320, back[0]);
  TEST_ASSERT_EQUAL_INT32(-130, back[1]);
  TEST_ASSERT_EQUAL_INT32(-86, back[5]);
}

void test_subfile_tolerates_unknown_keys_and_multiple_raw_lines() {
  const char* sample =
      "Filetype: Flipper SubGhz RAW File\n"
      "Version: 1\n"
      "Frequency: 868350000\n"
      "Preset: FuriHalSubGhzPresetFskDev238Async\n"
      "Vendor: PingequaCustom\n"      // unknown — should be ignored
      "Protocol: RAW\n"
      "RAW_Data: 100 -200 300 -400\n"
      "RAW_Data: 500 -600\n";
  yui::proto::SubHeader h{};
  std::vector<int32_t> v;
  bool ok = yui::proto::read_sub_to_vector(sample, std::strlen(sample), h, v);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT32(868'350'000, h.frequency_hz);
  TEST_ASSERT_TRUE(h.preset == yui::proto::SubPreset::FskDev238Async);
  TEST_ASSERT_EQUAL_size_t(6, v.size());
  TEST_ASSERT_EQUAL_INT32(500, v[4]);
  TEST_ASSERT_EQUAL_INT32(-600, v[5]);
}

void test_subfile_rejects_garbage() {
  const char* junk = "this is not a sub file\nat all\n";
  yui::proto::SubHeader h{};
  std::vector<int32_t> v;
  bool ok = yui::proto::read_sub_to_vector(junk, std::strlen(junk), h, v);
  TEST_ASSERT_FALSE(ok);  // missing Filetype tag
}

// ───── Phase 4 — Hydra RF apps (cap-detect smoke tests) ────────────────────
#include "yui/app/SubGhzScanApp.hpp"
#include "yui/app/SubGhzJammerApp.hpp"
#include "yui/app/Nrf24JammerApp.hpp"
#include "yui/app/RollJamApp.hpp"

// Helpers — inline Hal builder for these tests (existing tests use a
// similar pattern further up; we keep ours local to avoid coupling).
namespace {
struct RfHalFixture {
  yui::NativeDisplay  d{240, 135};
  yui::MockKeyboard   kb;
  yui::FakeClock      cl;
  yui::StderrLog      lg;
  yui::Hal hal{d, kb, cl, lg};
};
}  // namespace

void test_subghz_scan_with_present_cap_starts_scanning() {
  yui::NativeCc1101 c;
  RfHalFixture f;
  yui::SubGhzScanApp app(&c);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.mode() == yui::SubGhzScanApp::Mode::Scanning);
}

void test_subghz_scan_with_missing_cap_renders_dialog() {
  yui::NativeCc1101 c;
  c.set_present(false);
  RfHalFixture f;
  yui::SubGhzScanApp app(&c);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.mode() == yui::SubGhzScanApp::Mode::CapMissing);
  app.render(f.d);
  // Dialog text should appear in the rendered text stream.
  TEST_ASSERT_TRUE(f.d.all_text().find("Hydra not found") != std::string::npos);
}

void test_subghz_scan_null_radio_lands_in_cap_missing() {
  RfHalFixture f;
  yui::SubGhzScanApp app(nullptr);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.mode() == yui::SubGhzScanApp::Mode::CapMissing);
}

void test_subghz_jammer_enter_toggles_carrier_in_cw_mode() {
  yui::NativeCc1101 c;
  RfHalFixture f;
  yui::SubGhzJammerApp app(&c);
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(app.active());
  TEST_ASSERT_FALSE(c.carrier_on());
  // Press Enter — CW mode is the default (mode_=0); should turn carrier on.
  yui::KeyEvent ke{};
  ke.down = true;
  ke.key = yui::Key::Enter;
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.active());
  TEST_ASSERT_TRUE(c.carrier_on());
  // Press Enter again — back off.
  app.on_key(ke);
  TEST_ASSERT_FALSE(app.active());
  TEST_ASSERT_FALSE(c.carrier_on());
}

// Note: test_mousejack_generic_scan_* defined later, after MousejackApp.hpp
// is included around line 5010.

void test_nrf24_jammer_channel_flood_asserts_carrier() {
  yui::NativeNrf24 n;
  RfHalFixture f;
  yui::Nrf24JammerApp app(&n);
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(n.carrier_on());
  yui::KeyEvent ke{};
  ke.down = true;
  ke.key = yui::Key::Enter;
  app.on_key(ke);  // mode 0 = channel flood
  TEST_ASSERT_TRUE(n.carrier_on());
}

// ── Phase 4.5 fleshed-out logic ────────────────────────────────────────

#include "yui/app/SubGhzCaptureReplayApp.hpp"
#include "yui/app/SubGhzBruteApp.hpp"

void test_subghz_replay_lists_only_sub_files_in_sub_dir() {
  yui::NativeCc1101 c;
  yui::FakeFs fs;
  yui::FakeClock clock;
  fs.mkdir("/sub");
  yui::proto::SubHeader h{};
  h.frequency_hz = 433'920'000;
  int32_t t[] = {100, -100};
  fs.put_file("/sub/garage.sub", yui::proto::write_sub(h, t, 2));
  fs.put_file("/sub/notes.txt", "hello");
  RfHalFixture f;
  yui::SubGhzCaptureReplayApp app(&c, &fs, clock);
  app.on_enter(f.hal);
  app.set_view(yui::SubGhzCaptureReplayApp::View::Saved);
  TEST_ASSERT_TRUE(app.replay_mode() ==
                   yui::SubGhzCaptureReplayApp::ReplayMode::Browsing);
  TEST_ASSERT_EQUAL_INT(1, app.file_count());
}

void test_subghz_replay_enter_loads_and_transmits() {
  yui::NativeCc1101 c;
  yui::FakeFs fs;
  yui::FakeClock clock;
  fs.mkdir("/sub");
  yui::proto::SubHeader h{};
  h.frequency_hz = 433'920'000;
  int32_t t[] = {320, -130, 196, -118};
  fs.put_file("/sub/r.sub", yui::proto::write_sub(h, t, 4));
  RfHalFixture f;
  yui::SubGhzCaptureReplayApp app(&c, &fs, clock);
  app.on_enter(f.hal);
  app.set_view(yui::SubGhzCaptureReplayApp::View::Saved);
  yui::KeyEvent ke{};
  ke.down = true;
  ke.key  = yui::Key::Enter;
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.replay_mode() ==
                   yui::SubGhzCaptureReplayApp::ReplayMode::Done);
  TEST_ASSERT_TRUE(c.tx_call_count() >= 1);
  TEST_ASSERT_EQUAL_size_t(4, c.last_edges().size());
  TEST_ASSERT_EQUAL_INT32(320,  c.last_edges()[0]);
  TEST_ASSERT_EQUAL_INT32(-130, c.last_edges()[1]);
  TEST_ASSERT_EQUAL_INT32(196,  c.last_edges()[2]);
  TEST_ASSERT_EQUAL_INT32(-118, c.last_edges()[3]);
}

void test_subghz_replay_missing_cap_blocks_browsing() {
  yui::NativeCc1101 c;
  c.set_present(false);
  yui::FakeFs fs;
  yui::FakeClock clock;
  RfHalFixture f;
  yui::SubGhzCaptureReplayApp app(&c, &fs, clock);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.top_mode() ==
                   yui::SubGhzCaptureReplayApp::TopMode::CapMissing);
}

void test_subghz_cr_tab_toggles_view_when_idle() {
  yui::NativeCc1101 c;
  yui::FakeFs fs;
  yui::FakeClock clock;
  fs.mkdir("/sub");
  RfHalFixture f;
  yui::SubGhzCaptureReplayApp app(&c, &fs, clock);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.view() == yui::SubGhzCaptureReplayApp::View::Read);
  yui::KeyEvent ke{}; ke.down = true; ke.key = yui::Key::Tab;
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.view() == yui::SubGhzCaptureReplayApp::View::Saved);
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.view() == yui::SubGhzCaptureReplayApp::View::Read);
}

void test_subghz_cr_capture_appears_in_saved_listing() {
  yui::NativeCc1101 c;
  yui::FakeFs fs;
  yui::FakeClock clock;
  fs.mkdir("/sub");
  // Pre-stage a .sub file as if a prior capture had saved it. The
  // critical contract: switching to Saved view re-scans /sub/ and
  // surfaces files that weren't there at on_enter time.
  RfHalFixture f;
  yui::SubGhzCaptureReplayApp app(&c, &fs, clock);
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_INT(0, app.file_count());
  yui::proto::SubHeader h{};
  h.frequency_hz = 433'920'000;
  int32_t t[] = {200, -200};
  fs.put_file("/sub/late.sub", yui::proto::write_sub(h, t, 2));
  app.set_view(yui::SubGhzCaptureReplayApp::View::Saved);
  TEST_ASSERT_EQUAL_INT(1, app.file_count());
}

void test_subghz_brute_idle_until_enter_then_emits_packets() {
  yui::NativeCc1101 c;
  yui::FakeClock clock;
  RfHalFixture f;
  yui::SubGhzBruteApp app(&c, clock);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.mode() == yui::SubGhzBruteApp::Mode::Idle);

  yui::KeyEvent ke{};
  ke.down = true;
  ke.key = yui::Key::Enter;
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.mode() == yui::SubGhzBruteApp::Mode::Running);

  // First tick: pace gate fires (clock is at 0; last_step_ms_=0; diff=0;
  // gate is "< 100ms" → false branch, so it fires once at t=0). Subsequent
  // ticks within 100 ms get suppressed.
  app.tick(0);
  TEST_ASSERT_EQUAL_UINT32(1, app.sent());
  app.tick(50);
  TEST_ASSERT_EQUAL_UINT32(1, app.sent());  // still gated
  app.tick(150);
  TEST_ASSERT_EQUAL_UINT32(2, app.sent());

  TEST_ASSERT_TRUE(c.tx_call_count() == 2);
  // Verify a 24-bit packet shape (3 bytes per TX).
  TEST_ASSERT_EQUAL_size_t(3, c.last_tx().size());
}

void test_subghz_brute_step_up_doubles() {
  yui::NativeCc1101 c;
  yui::FakeClock clock;
  RfHalFixture f;
  yui::SubGhzBruteApp app(&c, clock);
  app.on_enter(f.hal);
  TEST_ASSERT_EQUAL_UINT32(1, app.step());
  yui::KeyEvent ke{};
  ke.down = true;
  ke.key = yui::Key::Up;
  app.on_key(ke);
  TEST_ASSERT_EQUAL_UINT32(2, app.step());
  app.on_key(ke);
  TEST_ASSERT_EQUAL_UINT32(4, app.step());
}

// ── Phase 4.6 — Mousejack + RollJam fleshed out ───────────────────────

#include "yui/app/MousejackApp.hpp"

void test_mousejack_missing_cap_dialog() {
  yui::NativeNrf24 n;
  n.set_present(false);
  yui::FakeClock clock;
  RfHalFixture f;
  yui::MousejackApp app(&n, clock);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.mode() == yui::MousejackApp::Mode::CapMissing);
  app.render(f.d);
  TEST_ASSERT_TRUE(f.d.all_text().find("Hydra not found") != std::string::npos);
}

void test_mousejack_generic_scan_mode_renders_heatmap() {
  // Generic Scan mode is the absorbed Nrf24Scan path: tab toggles
  // between the original Targets scan and a channel-heatmap scan.
  yui::NativeNrf24 n;
  yui::FakeClock clock;
  RfHalFixture f;
  yui::MousejackApp app(&n, clock);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.scan_type() == yui::MousejackApp::ScanType::Targets);
  yui::KeyEvent ke{}; ke.down = true; ke.key = yui::Key::Tab;
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.scan_type() == yui::MousejackApp::ScanType::Generic);
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.scan_type() == yui::MousejackApp::ScanType::Targets);
}

void test_mousejack_generic_scan_with_missing_cap_renders_dialog() {
  yui::NativeNrf24 n;
  n.set_present(false);
  yui::FakeClock clock;
  RfHalFixture f;
  yui::MousejackApp app(&n, clock);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.mode() == yui::MousejackApp::Mode::CapMissing);
}

void test_mousejack_enters_scan_and_walks_channels() {
  yui::NativeNrf24 n;
  yui::FakeClock clock;
  RfHalFixture f;
  yui::MousejackApp app(&n, clock);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.mode() == yui::MousejackApp::Mode::Scanning);
  TEST_ASSERT_TRUE(n.listening());
  const int before = app.current_channel();
  app.tick(0);
  app.tick(1);
  TEST_ASSERT_TRUE(app.current_channel() != before);
}

void test_mousejack_inject_emits_payload_bytes() {
  yui::NativeNrf24 n;
  yui::FakeClock clock;
  RfHalFixture f;
  yui::MousejackApp app(&n, clock);
  app.on_enter(f.hal);

  uint8_t addr[5] = {0xBB, 0x0A, 0xDC, 0xA5, 0x75};
  app.inject_target_for_test(addr, 5, 5);
  TEST_ASSERT_EQUAL_INT(1, app.target_count());

  yui::KeyEvent ke{};
  ke.down = true;
  ke.key = yui::Key::Enter;
  app.on_key(ke);  // Scanning → TargetList
  TEST_ASSERT_TRUE(app.mode() == yui::MousejackApp::Mode::TargetList);
  app.on_key(ke);  // TargetList → Injecting
  TEST_ASSERT_TRUE(app.mode() == yui::MousejackApp::Mode::Injecting);

  uint32_t t = 0;
  uint32_t fired = 0;
  while (app.mode() == yui::MousejackApp::Mode::Injecting && fired < 32) {
    app.tick(t);
    t += 51;
    ++fired;
  }
  TEST_ASSERT_TRUE(n.tx_call_count() >= 1);
}

void test_rolljam_idle_to_jam_asserts_carrier() {
  yui::NativeCc1101 c;
  yui::FakeClock clock;
  RfHalFixture f;
  yui::RollJamApp app(&c, clock);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.stage() == yui::RollJamApp::Stage::Idle);
  TEST_ASSERT_FALSE(c.carrier_on());

  yui::KeyEvent ke{};
  ke.down = true;
  ke.key = yui::Key::Enter;
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.stage() == yui::RollJamApp::Stage::Jamming);
  TEST_ASSERT_TRUE(c.carrier_on());
}

void test_rolljam_full_walk_drives_radio_through_states() {
  yui::NativeCc1101 c;
  yui::FakeClock clock;
  RfHalFixture f;
  yui::RollJamApp app(&c, clock);
  app.on_enter(f.hal);

  yui::KeyEvent ke{};
  ke.down = true;
  ke.key = yui::Key::Enter;

  app.on_key(ke);
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.stage() == yui::RollJamApp::Stage::Captured);
  TEST_ASSERT_FALSE(c.carrier_on());

  app.tick(0);
  TEST_ASSERT_EQUAL_size_t(0, app.capture_len());

  app.on_key(ke);
  TEST_ASSERT_TRUE(app.stage() == yui::RollJamApp::Stage::Replayed);
  TEST_ASSERT_TRUE(c.tx_call_count() >= 1);

  app.on_key(ke);
  TEST_ASSERT_TRUE(app.stage() == yui::RollJamApp::Stage::Idle);
  TEST_ASSERT_FALSE(c.carrier_on());
}

// ── Phase 4.7 — HydraStatus diagnostic app ─────────────────────────────

#include "yui/app/HydraStatusApp.hpp"

void test_hydra_status_reports_both_present() {
  yui::NativeCc1101 c;
  yui::NativeNrf24  n;
  RfHalFixture f;
  yui::HydraStatusApp app(&c, &n);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.cc_present());
  TEST_ASSERT_TRUE(app.nrf_present());
  app.render(f.d);
  TEST_ASSERT_TRUE(f.d.all_text().find("CC1101") != std::string::npos);
  TEST_ASSERT_TRUE(f.d.all_text().find("NRF24")  != std::string::npos);
  TEST_ASSERT_TRUE(f.d.all_text().find("OK") != std::string::npos);
}

void test_hydra_status_reports_partial_when_one_absent() {
  yui::NativeCc1101 c;
  yui::NativeNrf24  n;
  n.set_present(false);
  RfHalFixture f;
  yui::HydraStatusApp app(&c, &n);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.cc_present());
  TEST_ASSERT_FALSE(app.nrf_present());
  app.render(f.d);
  TEST_ASSERT_TRUE(f.d.all_text().find("PARTIAL") != std::string::npos);
}

void test_launcher_status_strip_includes_hydra_badge() {
  yui::AppRegistry reg;
  yui::SysProbe probe;
  probe.hydra_state = []() -> int { return 1; };  // both up
  yui::Launcher launcher(reg, probe);
  RfHalFixture f;
  launcher.on_enter(f.hal);
  launcher.render(f.d);
  // Carousel mode draws status floating; we just check the rendered
  // text stream contains the H+ badge.
  TEST_ASSERT_TRUE(f.d.all_text().find("H+") != std::string::npos);
}

void test_launcher_status_strip_partial_hydra_badge() {
  yui::AppRegistry reg;
  yui::SysProbe probe;
  probe.hydra_state = []() -> int { return 2; };  // partial
  yui::Launcher launcher(reg, probe);
  RfHalFixture f;
  launcher.on_enter(f.hal);
  launcher.render(f.d);
  TEST_ASSERT_TRUE(f.d.all_text().find("H~") != std::string::npos);
}

void test_launcher_status_strip_missing_hydra_badge() {
  yui::AppRegistry reg;
  yui::SysProbe probe;
  probe.hydra_state = []() -> int { return 3; };
  yui::Launcher launcher(reg, probe);
  RfHalFixture f;
  launcher.on_enter(f.hal);
  launcher.render(f.d);
  TEST_ASSERT_TRUE(f.d.all_text().find("H-") != std::string::npos);
}

// ── Phase 4.11 — wire-shape tests for the hardware-timing fixes ────────

void test_subghz_replay_uses_edge_toggle_not_packet_mode() {
  // Replay must drive transmit_raw_edges (edge-toggle path), not the
  // packet-mode transmit() it used in 4.5. Confirm by leaving last_tx
  // (packet mode) empty while last_edges (raw path) gets populated.
  yui::NativeCc1101 c;
  yui::FakeFs fs;
  yui::FakeClock clock;
  fs.mkdir("/sub");
  yui::proto::SubHeader h{};
  h.frequency_hz = 433'920'000;
  int32_t t[] = {500, -300, 200, -100, 700};
  fs.put_file("/sub/x.sub", yui::proto::write_sub(h, t, 5));
  RfHalFixture f;
  yui::SubGhzCaptureReplayApp app(&c, &fs, clock);
  app.on_enter(f.hal);
  app.set_view(yui::SubGhzCaptureReplayApp::View::Saved);
  yui::KeyEvent ke{};
  ke.down = true;
  ke.key  = yui::Key::Enter;
  app.on_key(ke);
  TEST_ASSERT_TRUE(c.last_tx().empty());
  TEST_ASSERT_EQUAL_size_t(5, c.last_edges().size());
  TEST_ASSERT_EQUAL_INT32(700, c.last_edges()[4]);
}

void test_mousejack_enables_promiscuous_before_scanning() {
  // The whole point of the 4.6→4.11 fix: scan must turn on
  // promiscuous mode so the chip can latch frames regardless of
  // pre-paired address.
  yui::NativeNrf24 n;
  yui::FakeClock clock;
  RfHalFixture f;
  yui::MousejackApp app(&n, clock);
  TEST_ASSERT_FALSE(n.is_promiscuous());
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(n.is_promiscuous());
  TEST_ASSERT_TRUE(n.listening());
}

void test_mousejack_on_exit_disables_promiscuous() {
  yui::NativeNrf24 n;
  yui::FakeClock clock;
  RfHalFixture f;
  yui::MousejackApp app(&n, clock);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(n.is_promiscuous());
  app.on_exit();
  TEST_ASSERT_FALSE(n.is_promiscuous());
}

void test_hydra_status_enter_reprobes() {
  yui::NativeCc1101 c;
  yui::NativeNrf24  n;
  c.set_present(false);
  RfHalFixture f;
  yui::HydraStatusApp app(&c, &n);
  app.on_enter(f.hal);
  TEST_ASSERT_FALSE(app.cc_present());
  // User plugs in the cap (simulated).
  c.set_present(true);
  yui::KeyEvent ke{};
  ke.down = true;
  ke.key = yui::Key::Enter;
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.cc_present());
}

// ───── Radio-presence indicators in app headers ────────────────────────────

namespace {
// Count "on-accent" pixels in the rightmost 60 px of the header band.
// That's where Chrome::radio_header paints the C/N indicators.
int count_indicator_pixels(NativeDisplay& d) {
  int n = 0;
  const int x0 = d.width() - 60;
  for (int y = 0; y < yui::ui::kHeaderH; ++y)
    for (int x = x0; x < d.width(); ++x)
      if (d.pixel_at(x, y) == yui::ui::kOnAccent) ++n;
  return n;
}
}  // namespace

void test_radio_header_filled_dot_when_cc1101_present() {
  NativeDisplay d;
  yui::ui::Chrome::radio_header(d, "Sub-GHz Scan", "433 MHz", 1, -1);
  // Filled 6x6 box paints 36 on-accent pixels (the native test font
  // doesn't render glyph pixels, so we only count the dot itself).
  TEST_ASSERT_GREATER_OR_EQUAL_INT(36, count_indicator_pixels(d));
}

void test_radio_header_hollow_dot_when_cc1101_absent() {
  NativeDisplay d_full, d_hollow;
  yui::ui::Chrome::radio_header(d_full,   "x", nullptr, 1, -1);
  yui::ui::Chrome::radio_header(d_hollow, "x", nullptr, 0, -1);
  // Hollow outline is strictly fewer pixels than the filled box.
  TEST_ASSERT_LESS_THAN_INT(count_indicator_pixels(d_full),
                            count_indicator_pixels(d_hollow));
}

void test_radio_header_hides_indicator_when_state_negative() {
  NativeDisplay d_one, d_two;
  yui::ui::Chrome::radio_header(d_one, "x", nullptr, 1, -1);  // CC only
  yui::ui::Chrome::radio_header(d_two, "x", nullptr, 1,  1);  // CC + NRF
  // Two visible dots paint strictly more on-accent pixels than one.
  TEST_ASSERT_LESS_THAN_INT(count_indicator_pixels(d_two),
                            count_indicator_pixels(d_one));
}

void test_radio_header_subtitle_does_not_collide_with_indicator() {
  // With a subtitle set, the indicator must still paint its dot — the
  // subtitle should be shifted left rather than overwriting the indicator
  // box on the right edge.
  NativeDisplay d;
  yui::ui::Chrome::radio_header(d, "App", "433 MHz", 1, -1);
  // The far-right edge of the header band must contain on-accent pixels
  // from the indicator (subtitle never reaches that far right).
  int hits = 0;
  for (int y = 0; y < yui::ui::kHeaderH; ++y)
    for (int x = d.width() - 12; x < d.width() - 4; ++x)
      if (d.pixel_at(x, y) == yui::ui::kOnAccent) ++hits;
  TEST_ASSERT_GREATER_THAN_INT(8, hits);
}

void test_rolljam_replay_uses_real_capture_when_present() {
  yui::NativeCc1101 c;
  yui::FakeClock clock;
  c.enqueue_rx({0xCA, 0xFE, 0xBA, 0xBE});
  RfHalFixture f;
  yui::RollJamApp app(&c, clock);
  app.on_enter(f.hal);
  yui::KeyEvent ke{};
  ke.down = true;
  ke.key = yui::Key::Enter;
  app.on_key(ke);
  app.on_key(ke);
  app.tick(0);
  TEST_ASSERT_EQUAL_size_t(4, app.capture_len());
  app.on_key(ke);
  TEST_ASSERT_EQUAL_size_t(4, c.last_tx().size());
  TEST_ASSERT_EQUAL_HEX8(0xCA, c.last_tx()[0]);
}

void test_subghz_jammer_noise_mode_writes_random_bytes() {
  yui::NativeCc1101 c;
  RfHalFixture f;
  yui::SubGhzJammerApp app(&c);
  app.on_enter(f.hal);
  // Move to Noise (mode 1).
  yui::KeyEvent down{};
  down.down = true;
  down.key = yui::Key::Down;
  app.on_key(down);
  TEST_ASSERT_EQUAL_INT(1, app.mode());
  // Activate.
  yui::KeyEvent enter{};
  enter.down = true;
  enter.key = yui::Key::Enter;
  app.on_key(enter);
  TEST_ASSERT_TRUE(app.active());
  // Tick should TX a 16-byte block.
  app.tick(0);
  TEST_ASSERT_EQUAL_size_t(16, c.last_tx().size());
}

void test_rolljam_walks_state_machine() {
  yui::NativeCc1101 c;
  yui::FakeClock clock;
  RfHalFixture f;
  yui::RollJamApp app(&c, clock);
  app.on_enter(f.hal);
  TEST_ASSERT_TRUE(app.stage() == yui::RollJamApp::Stage::Idle);
  yui::KeyEvent ke{};
  ke.down = true;
  ke.key = yui::Key::Enter;
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.stage() == yui::RollJamApp::Stage::Jamming);
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.stage() == yui::RollJamApp::Stage::Captured);
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.stage() == yui::RollJamApp::Stage::Replayed);
  app.on_key(ke);
  TEST_ASSERT_TRUE(app.stage() == yui::RollJamApp::Stage::Idle);
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
  RUN_TEST(test_splash_paints_white_background);
  RUN_TEST(test_splash_renders_red_koi_pixels_at_t0);
  RUN_TEST(test_splash_shimmer_advances_with_time);
  RUN_TEST(test_splash_writes_version_string);
  RUN_TEST(test_boot_animation_indexes_clamp_to_last_frame);
  RUN_TEST(test_boot_animation_calls_draw_png_per_frame);
  RUN_TEST(test_splash_flushes_once_per_frame);
  RUN_TEST(test_registry_starts_empty);
  RUN_TEST(test_registry_adds_apps);
  RUN_TEST(test_registry_rejects_null);
  RUN_TEST(test_launcher_cursor_starts_at_zero);
  RUN_TEST(test_launcher_down_advances_cursor);
  RUN_TEST(test_launcher_up_wraps);
  RUN_TEST(test_launcher_enter_sets_pending_launch);
  RUN_TEST(test_launcher_renders_carousel_tile_in_red);
  RUN_TEST(test_launcher_renders_dot_pager_for_current_category);
  RUN_TEST(test_launcher_with_sysprobe_renders_status_text);
  RUN_TEST(test_launcher_status_uses_wallclock_when_synced);
  RUN_TEST(test_launcher_status_falls_back_to_uptime_when_unsynced);
  RUN_TEST(test_launcher_without_sysprobe_skips_status);
  RUN_TEST(test_launcher_starts_in_categories_view);
  RUN_TEST(test_launcher_enter_on_empty_category_stays_in_categories);
  RUN_TEST(test_launcher_drills_into_category_on_enter);
  RUN_TEST(test_launcher_left_right_flip_categories);
  RUN_TEST(test_launcher_esc_returns_to_categories);
  RUN_TEST(test_remote_app_enter_toggles_runtime_only);
  RUN_TEST(test_remote_app_f_flips_persist_flag);
  RUN_TEST(test_shell_fn_plus_v_toggles_remote_and_persists);
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
  RUN_TEST(test_files_lists_root);
  RUN_TEST(test_files_enter_descends_into_dir);
  RUN_TEST(test_files_dotdot_at_subdir_pops_to_parent);
  RUN_TEST(test_files_no_dotdot_at_root);
  RUN_TEST(test_files_pop_restores_cursor);
  RUN_TEST(test_files_enter_on_file_opens_viewer);
  RUN_TEST(test_files_view_backspace_returns_to_list);
  RUN_TEST(test_ir_app_sends_nec_on_enter);
  RUN_TEST(test_ir_app_arrow_keys_change_selection);
  RUN_TEST(test_fft_dc_input_concentrates_in_bin_zero);
  RUN_TEST(test_fft_single_bin_sine);
  RUN_TEST(test_adv_decode_pos_keycode_1_is_tab_row1_col0);
  RUN_TEST(test_adv_make_event_letter_unshifted);
  RUN_TEST(test_adv_make_event_letter_shifted);
  RUN_TEST(test_adv_arrow_keys_are_primary);
  RUN_TEST(test_adv_esc_is_primary_on_backtick_key);
  RUN_TEST(test_settings_loads_default_brightness);
  RUN_TEST(test_settings_step_persists_to_storage);
  RUN_TEST(test_settings_left_decreases_brightness);
  RUN_TEST(test_settings_default_tz_is_utc);
  RUN_TEST(test_settings_tz_right_cycles_and_persists);
  RUN_TEST(test_settings_tz_left_wraps_to_last_preset);
  RUN_TEST(test_settings_tz_loads_persisted_value);
  RUN_TEST(test_tz_presets_index_of_known_value);
  RUN_TEST(test_settings_default_ntp_is_pool);
  RUN_TEST(test_settings_ntp_right_cycles_and_persists);
  RUN_TEST(test_settings_ntp_loads_persisted_value);
  RUN_TEST(test_ntp_presets_index_of_known_value);
  RUN_TEST(test_fakenet_wifi_connect_rejects_empty_ssid);
  RUN_TEST(test_fakenet_wifi_connect_immediate_marks_connected);
  RUN_TEST(test_fakenet_wifi_connect_async_path);
  RUN_TEST(test_fakenet_wifi_disconnect_clears_state);
  RUN_TEST(test_fakenet_ntp_sync_requires_connected);
  RUN_TEST(test_fakeclock_epoch_defaults_to_zero);
  RUN_TEST(test_fakeclock_set_epoch_round_trips);
  RUN_TEST(test_files_view_tab_toggles_hex);
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
  RUN_TEST(test_pineapple_app_enter_re_fetches);
  RUN_TEST(test_pineapple_tab_cycles_through_three_tabs);
  RUN_TEST(test_pineapple_no_creds_renders_unreachable_consistently);
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
  RUN_TEST(test_theme_default_is_kohaku);
  RUN_TEST(test_theme_apply_palette_swaps_live_tokens);
  RUN_TEST(test_settings_theme_row_persists_selection);
  RUN_TEST(test_settings_theme_row_loads_persisted_palette_on_enter);
  RUN_TEST(test_faraday_default_off_after_load);
  RUN_TEST(test_faraday_enable_persists_to_nvs_and_reloads);
  RUN_TEST(test_faraday_disable_clears_persisted_state);
  RUN_TEST(test_faraday_gps_guard_refuses_when_far_from_lab);
  RUN_TEST(test_faraday_gps_guard_allows_inside_lab_radius);
  RUN_TEST(test_faraday_no_fix_succeeds_even_with_lab_set);
  RUN_TEST(test_settings_faraday_row_toggles);
  RUN_TEST(test_selftest_initial_state_pending);
  RUN_TEST(test_selftest_run_with_no_wiring_marks_skip_for_optional);
  RUN_TEST(test_selftest_full_wiring_passes_all_present);
  RUN_TEST(test_selftest_storage_rw_round_trip);
  RUN_TEST(test_selftest_cap_absent_marks_skip_not_fail);
  RUN_TEST(test_settings_boot_selftest_default_off);
  RUN_TEST(test_settings_boot_selftest_toggle_persists);
  RUN_TEST(test_bblink_probe_pending_until_run);
  RUN_TEST(test_bblink_probe_no_advertise_marks_scan_fail);
  RUN_TEST(test_bblink_probe_full_path_passes_all_three);
  RUN_TEST(test_bblink_probe_missing_nus_marks_gatt_fail);
  RUN_TEST(test_ble_rawtx_nimble_rail_accepts_only_adv_channels);
  RUN_TEST(test_ble_rawtx_nimble_rail_records_payload_and_count);
  RUN_TEST(test_ble_rawtx_nrf24_channel_mapping_is_correct);
  RUN_TEST(test_ble_rawtx_nrf24_rail_rejects_invalid_channel);
  RUN_TEST(test_ble_rawtx_continuous_start_stop_lifecycle);
  RUN_TEST(test_ble_rawtx_dual_rail_independent_counters);
  RUN_TEST(test_ble_rawtx_payload_over_31_bytes_is_rejected);
  RUN_TEST(test_ble_rawtx_cap_missing_blocks_tx);
  RUN_TEST(test_ble_rawtx_channel_split_no_overlap);
  RUN_TEST(test_ble_rawtx_rail_names);
  RUN_TEST(test_rfchaos_refuses_arm_without_faraday);
  RUN_TEST(test_rfchaos_arms_when_faraday_on_and_targets_present);
  RUN_TEST(test_rfchaos_fire_rate_scales_with_intensity);
  RUN_TEST(test_rfchaos_natural_completion_at_deadline);
  RUN_TEST(test_rfchaos_fires_targets_round_robin_with_seed);
  RUN_TEST(test_rfchaos_writes_log_on_stop);
  RUN_TEST(test_rfchaos_intensity_clamps);
  RUN_TEST(test_rfchaos_duration_minutes);
  RUN_TEST(test_eapol_m1_request_has_correct_layout);
  RUN_TEST(test_eapol_extract_pmkid_finds_kde);
  RUN_TEST(test_eapol_extract_pmkid_returns_false_when_absent);
  RUN_TEST(test_handshake_app_tab_toggles_mode);
  RUN_TEST(test_handshake_app_counts_pmkid_in_injected_eapol);
  RUN_TEST(test_mousejack_pace_clamps_when_faraday_off);
  RUN_TEST(test_mousejack_pace_unlocks_when_faraday_on);
  RUN_TEST(test_brute_pace_clamps_when_faraday_off);
  RUN_TEST(test_brute_pace_unlocks_when_faraday_on);
  RUN_TEST(test_blespam_default_rail_is_nimble);
  RUN_TEST(test_blespam_set_nrf24_rail_promotes_to_both);
  RUN_TEST(test_blespam_dual_rail_drives_both_engines);
  RUN_TEST(test_blespam_tab_cycles_rail_when_idle);
  RUN_TEST(test_blejammer_dual_rail_starts_continuous);
  RUN_TEST(test_blejammer_disable_stops_both_rails);
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
  RUN_TEST(test_deauth_b_key_toggles_backend);
  // v0.2 stretch — Track C
  RUN_TEST(test_ble_spam_off_by_default);
  RUN_TEST(test_ble_spam_fn_enter_toggles_and_advertises);
  RUN_TEST(test_ble_spam_cycles_payloads);
  RUN_TEST(test_ble_spam_disable_stops_advertiser);
  // Bruce-parity sweep
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
  RUN_TEST(test_captive_portal_fn_tab_writes_captures_to_sd);
  RUN_TEST(test_rogue_ap_tab_cycles_three_modes);
  RUN_TEST(test_rogue_ap_switching_mode_stops_captive);
  RUN_TEST(test_captive_portal_backspace_stops_ap);
  // SatTracker
  RUN_TEST(test_tle_parses_iss_sample);
  RUN_TEST(test_tle_compressed_exp_decode);
  RUN_TEST(test_tle_rejects_mismatched_catalog);
  RUN_TEST(test_tle_year_window);
  RUN_TEST(test_vec3_basic_ops);
  RUN_TEST(test_vec3_normalized_unit);
  RUN_TEST(test_jd_from_unix_epoch_match);
  RUN_TEST(test_gstime_known_value);
  RUN_TEST(test_propagator_init_succeeds);
  RUN_TEST(test_propagator_position_at_epoch_is_finite_and_leo);
  RUN_TEST(test_propagator_position_changes_with_time);
  RUN_TEST(test_propagator_one_orbit_returns_close_to_start);
  RUN_TEST(test_topo_satellite_overhead_is_90deg_elevation);
  RUN_TEST(test_topo_doppler_sign_convention);
  RUN_TEST(test_pass_predictor_finds_a_pass_within_24h);
  RUN_TEST(test_sattracker_loads_default_tles_when_no_sd);
  RUN_TEST(test_sattracker_loads_tles_from_sd_if_present);
  RUN_TEST(test_sattracker_arrow_keys_move_cursor_in_list);
  RUN_TEST(test_sattracker_enter_drills_into_detail);
  RUN_TEST(test_sattracker_tab_in_detail_starts_live);
  RUN_TEST(test_sattracker_live_writes_doppler_to_radio);
  // v0.3 review fixes — regression coverage
  RUN_TEST(test_pass_predictor_starting_mid_pass_pins_aos_to_search_start);
  RUN_TEST(test_sattracker_accepts_two_line_tle_no_name);
  // v0.3 gap fills — sat module
  RUN_TEST(test_vec3_mag_and_mag2);
  RUN_TEST(test_jd_from_unix_round_trip_to_j2000);
  RUN_TEST(test_topo_observer_ecef_at_known_points);
  RUN_TEST(test_topo_eci_to_ecef_zero_gmst_is_identity);
  // v0.3 gap fills — fake HAL fixtures
  RUN_TEST(test_fake_ble_central_connect_enumerate_read);
  RUN_TEST(test_fake_wifi_monitor_filters_by_pkt_type);
  // Phase 3 — Hydra HAL + .sub file format
  RUN_TEST(test_native_cc1101_present_by_default_and_can_be_disabled);
  RUN_TEST(test_native_cc1101_freq_validates_band);
  RUN_TEST(test_native_cc1101_transmit_records_payload);
  RUN_TEST(test_native_cc1101_carrier_toggles);
  RUN_TEST(test_native_cc1101_receive_drains_queue);
  RUN_TEST(test_native_nrf24_channel_validates);
  RUN_TEST(test_native_nrf24_address_width_3_to_5);
  RUN_TEST(test_native_nrf24_listen_toggles_state);
  RUN_TEST(test_subfile_round_trip_raw_timings);
  RUN_TEST(test_subfile_tolerates_unknown_keys_and_multiple_raw_lines);
  RUN_TEST(test_subfile_rejects_garbage);
  // Phase 4 — Hydra RF apps
  RUN_TEST(test_subghz_scan_with_present_cap_starts_scanning);
  RUN_TEST(test_subghz_scan_with_missing_cap_renders_dialog);
  RUN_TEST(test_subghz_scan_null_radio_lands_in_cap_missing);
  RUN_TEST(test_subghz_jammer_enter_toggles_carrier_in_cw_mode);
  RUN_TEST(test_mousejack_generic_scan_mode_renders_heatmap);
  RUN_TEST(test_mousejack_generic_scan_with_missing_cap_renders_dialog);
  RUN_TEST(test_nrf24_jammer_channel_flood_asserts_carrier);
  RUN_TEST(test_rolljam_walks_state_machine);
  // Phase 4.5 — fleshed-out RF apps
  RUN_TEST(test_subghz_replay_lists_only_sub_files_in_sub_dir);
  RUN_TEST(test_subghz_replay_enter_loads_and_transmits);
  RUN_TEST(test_subghz_replay_missing_cap_blocks_browsing);
  RUN_TEST(test_subghz_cr_tab_toggles_view_when_idle);
  RUN_TEST(test_subghz_cr_capture_appears_in_saved_listing);
  RUN_TEST(test_subghz_brute_idle_until_enter_then_emits_packets);
  RUN_TEST(test_subghz_brute_step_up_doubles);
  RUN_TEST(test_subghz_jammer_noise_mode_writes_random_bytes);
  // Phase 4.6 — Mousejack + RollJam fleshed out
  RUN_TEST(test_mousejack_missing_cap_dialog);
  RUN_TEST(test_mousejack_enters_scan_and_walks_channels);
  RUN_TEST(test_mousejack_inject_emits_payload_bytes);
  RUN_TEST(test_rolljam_idle_to_jam_asserts_carrier);
  RUN_TEST(test_rolljam_full_walk_drives_radio_through_states);
  RUN_TEST(test_rolljam_replay_uses_real_capture_when_present);
  // Phase 4.7 — HydraStatus diagnostic
  RUN_TEST(test_hydra_status_reports_both_present);
  RUN_TEST(test_hydra_status_reports_partial_when_one_absent);
  RUN_TEST(test_launcher_status_strip_includes_hydra_badge);
  RUN_TEST(test_launcher_status_strip_partial_hydra_badge);
  RUN_TEST(test_launcher_status_strip_missing_hydra_badge);
  // Phase 4.11 — wire-shape tests for the hardware-timing fixes
  RUN_TEST(test_subghz_replay_uses_edge_toggle_not_packet_mode);
  RUN_TEST(test_mousejack_enables_promiscuous_before_scanning);
  RUN_TEST(test_mousejack_on_exit_disables_promiscuous);
  RUN_TEST(test_hydra_status_enter_reprobes);
  RUN_TEST(test_radio_header_filled_dot_when_cc1101_present);
  RUN_TEST(test_radio_header_hollow_dot_when_cc1101_absent);
  RUN_TEST(test_radio_header_hides_indicator_when_state_negative);
  RUN_TEST(test_radio_header_subtitle_does_not_collide_with_indicator);
  return UNITY_END();
}

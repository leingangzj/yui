#pragma once
// FaradayMode — process-wide flag that signals "all RF in this room is
// shielded; aggressive RF apps may strip their politeness gates and run
// at full duty cycle."
//
// Storage: NVS key "system.faraday_mode" (bool stored as int 0/1).
// Optional lab location: NVS keys "system.lab_lat" / "system.lab_lon"
// (floats stored as ints in micro-degrees) used by Phase 5.x apps that
// want to refuse to enable Faraday Mode unless within ~50 m of lab.
//
// This header keeps the runtime state in a single TU-shared variable so
// any app can check it without dependency injection. Tests reset the
// state between cases by calling reset_for_test().
#include "yui/hal/IStorage.hpp"
#include <cstdint>
#include <cmath>

namespace yui {

class FaradayMode {
 public:
  static constexpr const char* kNvsKey   = "sys.faraday";
  static constexpr const char* kNvsLat   = "sys.lab_lat";  // micro-degrees
  static constexpr const char* kNvsLon   = "sys.lab_lon";
  static constexpr int32_t      kLabRadiusMeters = 50;

  // Load persisted state from NVS into the singleton. Call once at boot
  // and again any time storage may have changed underneath us.
  static void load(IStorage& store) {
    int32_t v = 0;
    store.get_int(kNvsKey, v, 0);
    active_ = (v != 0);
    int32_t lat_uDeg = 0, lon_uDeg = 0;
    store.get_int(kNvsLat, lat_uDeg, 0);
    store.get_int(kNvsLon, lon_uDeg, 0);
    lab_lat_ = static_cast<double>(lat_uDeg) / 1.0e6;
    lab_lon_ = static_cast<double>(lon_uDeg) / 1.0e6;
    has_lab_loc_ = (lat_uDeg != 0 || lon_uDeg != 0);
  }

  // True iff Faraday Mode is currently armed. Aggressive RF apps check
  // this to decide whether to strip arm gates / uncap duty cycles.
  static bool is_active() { return active_; }

  // Try to enable. Returns true on success. Refuses if a lab location
  // is set AND a current GPS fix has been provided AND the fix is
  // outside kLabRadiusMeters of the lab — belt-and-suspenders against
  // forgetting to disable outdoors. With no lab location stored or no
  // current GPS fix, succeeds unconditionally.
  static bool enable(IStorage& store, bool gps_fix_available = false,
                     double gps_lat_deg = 0.0, double gps_lon_deg = 0.0) {
    if (active_) return true;
    if (has_lab_loc_ && gps_fix_available) {
      const double dm = haversine_meters_(gps_lat_deg, gps_lon_deg,
                                          lab_lat_, lab_lon_);
      if (dm > static_cast<double>(kLabRadiusMeters)) return false;
    }
    active_ = true;
    store.put_int(kNvsKey, 1);
    return true;
  }

  static void disable(IStorage& store) {
    active_ = false;
    store.put_int(kNvsKey, 0);
  }

  static void set_lab_location(IStorage& store, double lat_deg, double lon_deg) {
    lab_lat_ = lat_deg;
    lab_lon_ = lon_deg;
    has_lab_loc_ = true;
    store.put_int(kNvsLat, static_cast<int32_t>(lat_deg * 1.0e6));
    store.put_int(kNvsLon, static_cast<int32_t>(lon_deg * 1.0e6));
  }

  static bool has_lab_location() { return has_lab_loc_; }

  // Test-only: wipe the singleton state without touching NVS.
  static void reset_for_test() {
    active_ = false; has_lab_loc_ = false; lab_lat_ = 0; lab_lon_ = 0;
  }

 private:
  static inline bool   active_       = false;
  static inline bool   has_lab_loc_  = false;
  static inline double lab_lat_      = 0;
  static inline double lab_lon_      = 0;

  static double haversine_meters_(double lat1, double lon1,
                                  double lat2, double lon2) {
    constexpr double kR    = 6371000.0;
    constexpr double kPi   = 3.14159265358979323846;
    const double rad = kPi / 180.0;
    const double dLat = (lat2 - lat1) * rad;
    const double dLon = (lon2 - lon1) * rad;
    const double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                     std::cos(lat1 * rad) * std::cos(lat2 * rad) *
                     std::sin(dLon / 2) * std::sin(dLon / 2);
    const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return kR * c;
  }
};

}  // namespace yui

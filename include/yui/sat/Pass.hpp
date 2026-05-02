#pragma once
// Pass predictor — given a propagator, observer, and start time,
// find the next AOS / max-elevation / LOS for the upcoming pass.
//
// Algorithm: coarse step (~30 s) to find sign change in elevation,
// then bisect to pin AOS/LOS within ~1 second. Max-elevation found
// by tracking peak across the coarse sweep.
#include "yui/sat/Propagator.hpp"
#include "yui/sat/Topo.hpp"

namespace yui::sat {

struct PassInfo {
  bool   found         = false;
  double aos_jd        = 0;       // Acquisition of signal (rise above horizon)
  double max_jd        = 0;
  double los_jd        = 0;       // Loss of signal
  double max_elevation_deg = 0;
  double aos_azimuth_deg   = 0;
  double los_azimuth_deg   = 0;
};

namespace pass_detail {

inline double elevation_at(const Propagator& prop,
                           const ObserverGeodetic& obs,
                           double jd, double seconds_since_epoch) {
  StateVector sv;
  if (!prop.propagate(seconds_since_epoch, sv)) return -90.0;
  return look_angles(sv, obs, jd).elevation_deg;
}

}  // namespace pass_detail

// Find the next pass with elevation >= horizon_deg, starting from
// jd_start (UT). Searches up to search_hours ahead. Returns found=true
// if a pass is found.
inline PassInfo predict_next_pass(const Propagator& prop,
                                  const ObserverGeodetic& obs,
                                  double jd_start,
                                  double search_hours = 24.0,
                                  double horizon_deg = 0.0,
                                  double coarse_step_sec = 30.0) {
  PassInfo info;
  if (!prop.inited()) return info;

  // The TLE epoch in JD
  const double jd_epoch = jd_from_year_day(prop.epoch_year(), prop.epoch_day());
  const double t0_sec  = (jd_start - jd_epoch) * kSecsPerDay;
  const double t_end   = t0_sec + search_hours * 3600.0;

  // Coarse sweep
  double t = t0_sec;
  double prev_el = pass_detail::elevation_at(prop, obs,
                       jd_epoch + t / kSecsPerDay, t);
  bool   in_pass = (prev_el >= horizon_deg);
  double aos_t = -1, los_t = -1;
  double max_el = -90, max_t = t0_sec;
  // If the search starts mid-pass, AOS already happened — pin aos_t to
  // the search start so a later LOS produces a valid PassInfo instead
  // of leaking the -1 sentinel into info.aos_jd.
  if (in_pass) {
    aos_t  = t0_sec;
    max_el = prev_el; max_t = t0_sec;
    StateVector sv0;
    if (prop.propagate(t0_sec, sv0)) {
      info.aos_azimuth_deg =
          look_angles(sv0, obs, jd_epoch + t0_sec / kSecsPerDay).azimuth_deg;
    }
  }

  while (t < t_end) {
    t += coarse_step_sec;
    const double jd_now = jd_epoch + t / kSecsPerDay;
    const double el = pass_detail::elevation_at(prop, obs, jd_now, t);

    if (!in_pass && el >= horizon_deg) {
      // Bisect for AOS between (t - step) and t
      double lo = t - coarse_step_sec, hi = t;
      for (int i = 0; i < 30; ++i) {
        const double mid = 0.5 * (lo + hi);
        const double e = pass_detail::elevation_at(prop, obs,
                            jd_epoch + mid / kSecsPerDay, mid);
        if (e >= horizon_deg) hi = mid; else lo = mid;
        if (hi - lo < 1.0) break;
      }
      aos_t = hi;
      // capture AOS azimuth
      StateVector sv;
      prop.propagate(aos_t, sv);
      info.aos_azimuth_deg =
          look_angles(sv, obs, jd_epoch + aos_t / kSecsPerDay).azimuth_deg;
      in_pass = true;
      max_el = el; max_t = t;
    } else if (in_pass && el < horizon_deg) {
      // Bisect for LOS
      double lo = t - coarse_step_sec, hi = t;
      for (int i = 0; i < 30; ++i) {
        const double mid = 0.5 * (lo + hi);
        const double e = pass_detail::elevation_at(prop, obs,
                            jd_epoch + mid / kSecsPerDay, mid);
        if (e < horizon_deg) hi = mid; else lo = mid;
        if (hi - lo < 1.0) break;
      }
      los_t = hi;
      StateVector sv;
      prop.propagate(los_t, sv);
      info.los_azimuth_deg =
          look_angles(sv, obs, jd_epoch + los_t / kSecsPerDay).azimuth_deg;
      info.found      = true;
      info.aos_jd     = jd_epoch + aos_t / kSecsPerDay;
      info.los_jd     = jd_epoch + los_t / kSecsPerDay;
      info.max_jd     = jd_epoch + max_t / kSecsPerDay;
      info.max_elevation_deg = max_el;
      return info;
    } else if (in_pass && el > max_el) {
      max_el = el; max_t = t;
    }
    prev_el = el;
  }
  return info;
}

}  // namespace yui::sat

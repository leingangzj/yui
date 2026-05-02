#pragma once
// Convert ECI satellite state into observer-relative topocentric
// quantities (azimuth, elevation, range, range-rate) and Doppler
// shift for a given carrier frequency.
//
// Math: observer in geodetic lat/lon/alt → ECEF position, then ECI →
// ECEF rotation by GMST, then ECEF → ENU (east-north-up) at observer.
#include "yui/sat/Vec3.hpp"
#include "yui/sat/Time.hpp"
#include "yui/sat/Propagator.hpp"
#include <cmath>

namespace yui::sat {

struct ObserverGeodetic {
  double lat_deg  = 0;   // +N
  double lon_deg  = 0;   // +E
  double alt_m    = 0;
};

struct LookAngles {
  double azimuth_deg   = 0;   // 0..360, north=0, east=90
  double elevation_deg = 0;   // -90..+90
  double range_km      = 0;
  double range_rate_kms = 0;  // closing positive? we return signed: + = receding
};

// Observer position in ECEF (km). Spherical Earth approximation since
// our position errors at the antenna are much smaller than the
// SGP4-class propagation errors.
inline Vec3 observer_ecef_km(const ObserverGeodetic& obs) {
  const double lat = obs.lat_deg * kDeg2Rad;
  const double lon = obs.lon_deg * kDeg2Rad;
  const double r   = kEarthRadius_km + obs.alt_m / 1000.0;
  return {r * std::cos(lat) * std::cos(lon),
          r * std::cos(lat) * std::sin(lon),
          r * std::sin(lat)};
}

// Rotate an ECI vector to ECEF given GMST (radians).
inline Vec3 eci_to_ecef(const Vec3& eci, double gmst_rad) {
  const double c = std::cos(gmst_rad);
  const double s = std::sin(gmst_rad);
  return {  c * eci.x + s * eci.y,
           -s * eci.x + c * eci.y,
            eci.z };
}

// Compute look-angles given satellite ECI state, observer geodetic
// position, and JD UTC at evaluation time.
inline LookAngles look_angles(const StateVector& sat_eci,
                              const ObserverGeodetic& obs,
                              double jd_utc) {
  const double gmst = gstime(jd_utc);

  // Sat in ECEF
  const Vec3 sat_ecef = eci_to_ecef(sat_eci.position, gmst);
  // Sat velocity ECI → ECEF: subtract Earth rotation (ω×r), then rotate.
  Vec3 vel_inertial_ecef = eci_to_ecef(sat_eci.velocity, gmst);
  // Subtract Earth rotation contribution (in ECEF after rotation):
  vel_inertial_ecef.x -= -kSidRate * sat_ecef.y;
  vel_inertial_ecef.y -= +kSidRate * sat_ecef.x;
  // (z unchanged)

  const Vec3 obs_ecef = observer_ecef_km(obs);
  const Vec3 rho      = sat_ecef - obs_ecef;        // line-of-sight (ECEF)

  // Rotate rho into ENU at observer
  const double lat = obs.lat_deg * kDeg2Rad;
  const double lon = obs.lon_deg * kDeg2Rad;
  const double sl = std::sin(lat), cl = std::cos(lat);
  const double sn = std::sin(lon), cn = std::cos(lon);

  const double e =  -sn * rho.x +  cn * rho.y;
  const double n = -sl*cn * rho.x - sl*sn * rho.y +  cl * rho.z;
  const double u =  cl*cn * rho.x + cl*sn * rho.y +  sl * rho.z;

  LookAngles la;
  la.range_km      = std::sqrt(e * e + n * n + u * u);
  la.elevation_deg = std::asin(u / la.range_km) * kRad2Deg;
  double az = std::atan2(e, n) * kRad2Deg;
  if (az < 0) az += 360.0;
  la.azimuth_deg = az;

  // Range-rate from radial component of relative velocity.
  // Relative velocity ECEF = sat_velocity_ecef - 0 (observer fixed in ECEF).
  const Vec3 rho_hat = rho.normalized();
  la.range_rate_kms = vel_inertial_ecef.dot(rho_hat);

  return la;
}

// Doppler shift in Hz. Positive Δf when satellite is approaching
// (range-rate negative). f_obs = f_emit * (1 - vr/c).
inline double doppler_hz(double carrier_hz, double range_rate_kms) {
  // f_obs - f_emit = -f_emit * vr/c   (vr positive = receding → red-shift)
  return -carrier_hz * range_rate_kms / kSpeedLight_kms;
}

}  // namespace yui::sat

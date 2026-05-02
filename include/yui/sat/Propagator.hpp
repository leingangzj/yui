#pragma once
// Orbital propagator for SatTracker.
//
// SCOPE: This is a simplified Keplerian propagator with J2 secular
// perturbations on RAAN, argument of perigee, and mean anomaly. It is
// NOT full SGP4 — short-period terms, lunar/solar perturbations, and
// drag are omitted. This puts us in the same accuracy class as G3RUH's
// Plan13 (~tens of km position error over 24 h on LEO). For the
// amateur-satellite use case (next-pass az/el to 1° + Doppler-shift
// tracking on FM birds), this is plenty.
//
// If we ever need ±5 km over a week, vendor the Vallado/NORAD SGP4
// reference (public domain) and swap the implementation. The public
// API of this class is designed to make that swap transparent.
//
// References:
//   Hoots & Roehrich 1980 (Spacetrack Report #3)
//   Vallado, "Fundamentals of Astrodynamics" 4th ed.
//   G3RUH, "Plan-13 Satellite Position Calculation Program" (1983)
#include "yui/sat/Tle.hpp"
#include "yui/sat/Vec3.hpp"
#include "yui/sat/Time.hpp"
#include <cmath>

namespace yui::sat {

// WGS-72 (matching SGP4) — slightly different from WGS-84 used by GPS.
// Using WGS-72 so future swap to true SGP4 doesn't shift things.
constexpr double kEarthRadius_km = 6378.135;
constexpr double kMu_km3_s2      = 398600.8;       // GM
constexpr double kJ2             = 0.00108262998905;
constexpr double kSidRate        = 0.00007292115147;  // rad/s — Earth rotation
constexpr double kSpeedLight_kms = 299792.458;

struct StateVector {
  Vec3 position;   // ECI, km
  Vec3 velocity;   // ECI, km/s
};

class Propagator {
public:
  bool init(const TleElements& tle) {
    // Unit conversions
    i0_     = tle.inclination_deg  * kDeg2Rad;
    e0_     = tle.eccentricity;
    omega0_ = tle.arg_perigee_deg  * kDeg2Rad;
    Omega0_ = tle.raan_deg         * kDeg2Rad;
    M0_     = tle.mean_anomaly_deg * kDeg2Rad;

    // Mean motion: TLE gives revs/day. Convert to rad/sec.
    n0_ = tle.mean_motion * kTwoPi / kSecsPerDay;
    if (n0_ <= 0) return false;

    // Semi-major axis from Kepler's third law: a = (mu / n²)^(1/3)
    a0_ = std::pow(kMu_km3_s2 / (n0_ * n0_), 1.0 / 3.0);

    // J2 secular rates (Vallado eqs. 9-37..9-39, simplified — no Brouwer
    // mean-element conversion; we treat TLE n0 as the original mean motion).
    if (e0_ >= 1.0) return false;
    const double p   = a0_ * (1.0 - e0_ * e0_);
    const double cos_i = std::cos(i0_);
    const double cos2_i = cos_i * cos_i;
    const double common = 1.5 * kJ2 * n0_
                        * (kEarthRadius_km / p) * (kEarthRadius_km / p);

    dot_Omega_ = -common * cos_i;
    dot_omega_ =  common * (2.5 * cos2_i - 0.5);
    // Mean motion + secular addition to mean anomaly
    const double sqrt_1me2 = std::sqrt(1.0 - e0_ * e0_);
    dot_M_     = n0_ + 0.5 * common * sqrt_1me2 * (3.0 * cos2_i - 1.0);

    epoch_year_ = tle.epoch_year;
    epoch_day_  = tle.epoch_day;
    inited_ = true;
    return true;
  }

  bool inited() const { return inited_; }

  // Propagate t seconds from TLE epoch. Output: ECI state in km, km/s.
  bool propagate(double seconds_since_epoch, StateVector& out) const {
    if (!inited_) return false;

    // Updated elements
    const double M     = M0_ + dot_M_ * seconds_since_epoch;
    const double omega = omega0_ + dot_omega_ * seconds_since_epoch;
    const double Omega = Omega0_ + dot_Omega_ * seconds_since_epoch;

    // Solve Kepler's equation E - e sin(E) = M (radians) via Newton.
    double E = M;
    for (int it = 0; it < 12; ++it) {
      const double f  = E - e0_ * std::sin(E) - M;
      const double fp = 1.0 - e0_ * std::cos(E);
      const double dE = f / fp;
      E -= dE;
      if (std::fabs(dE) < 1.0e-12) break;
    }

    const double cosE = std::cos(E);
    const double sinE = std::sin(E);
    const double sqrt_1me2 = std::sqrt(1.0 - e0_ * e0_);

    // True anomaly
    const double nu = std::atan2(sqrt_1me2 * sinE, cosE - e0_);
    // Radius (km)
    const double r  = a0_ * (1.0 - e0_ * cosE);
    // Argument of latitude
    const double u  = omega + nu;

    // Position in ECI via 3-1-3 rotation (Vallado eq. 4-1).
    const double cos_u = std::cos(u);
    const double sin_u = std::sin(u);
    const double cos_O = std::cos(Omega);
    const double sin_O = std::sin(Omega);
    const double cos_i = std::cos(i0_);
    const double sin_i = std::sin(i0_);

    out.position.x = r * (cos_O * cos_u - sin_O * sin_u * cos_i);
    out.position.y = r * (sin_O * cos_u + cos_O * sin_u * cos_i);
    out.position.z = r * (sin_u * sin_i);

    // Velocity: in perifocal frame, vr = sqrt(mu/p) e sin(ν),
    //                              vt = sqrt(mu/p) (1 + e cos(ν))
    const double p = a0_ * (1.0 - e0_ * e0_);
    const double sqrt_mu_p = std::sqrt(kMu_km3_s2 / p);
    const double vr = sqrt_mu_p * e0_ * std::sin(nu);
    const double vt = sqrt_mu_p * (1.0 + e0_ * std::cos(nu));

    // Transform perifocal velocity (radial + transverse) into ECI.
    // dRhat/dt direction = (-sin(u), cos(u), 0) in orbital plane.
    // Position direction unit vector in ECI:
    const Vec3 r_hat = out.position.normalized();
    // Tangential unit: derivative of (cos u, sin u) in orbital plane
    // mapped through the same rotation.
    Vec3 t_hat;
    t_hat.x = (cos_O * (-sin_u) - sin_O * cos_u * cos_i);
    t_hat.y = (sin_O * (-sin_u) + cos_O * cos_u * cos_i);
    t_hat.z = (cos_u * sin_i);

    out.velocity.x = vr * r_hat.x + vt * t_hat.x;
    out.velocity.y = vr * r_hat.y + vt * t_hat.y;
    out.velocity.z = vr * r_hat.z + vt * t_hat.z;

    return true;
  }

  int    epoch_year() const { return epoch_year_; }
  double epoch_day()  const { return epoch_day_; }

private:
  bool   inited_     = false;
  double i0_         = 0;
  double e0_         = 0;
  double omega0_     = 0;
  double Omega0_     = 0;
  double M0_         = 0;
  double n0_         = 0;
  double a0_         = 0;
  double dot_Omega_  = 0;
  double dot_omega_  = 0;
  double dot_M_      = 0;
  int    epoch_year_ = 0;
  double epoch_day_  = 0;
};

}  // namespace yui::sat

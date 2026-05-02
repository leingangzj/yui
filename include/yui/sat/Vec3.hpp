#pragma once
// 3-component double-precision vector. Used for ECI/ECEF/topocentric
// position and velocity in the SatTracker math.
#include <cmath>

namespace yui::sat {

struct Vec3 {
  double x = 0, y = 0, z = 0;

  Vec3() = default;
  constexpr Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

  Vec3  operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
  Vec3  operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
  Vec3  operator*(double s)      const { return {x * s, y * s, z * s}; }

  double mag()  const { return std::sqrt(x * x + y * y + z * z); }
  double mag2() const { return x * x + y * y + z * z; }
  double dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
  Vec3   normalized() const {
    const double m = mag();
    if (m == 0) return {0, 0, 0};
    return {x / m, y / m, z / m};
  }
};

}  // namespace yui::sat

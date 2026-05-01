#pragma once
#include "yui/hal/IImu.hpp"

namespace yui {

class FakeImu : public IImu {
public:
  bool init() override { return true; }
  bool read_accel(AccelXYZ& out) override { out = accel_; return true; }
  bool read_gyro(GyroXYZ& out)   override { out = gyro_;  return true; }

  void set_accel(float x, float y, float z) { accel_ = {x, y, z}; }
  void set_gyro(float x, float y, float z)  { gyro_  = {x, y, z}; }

private:
  AccelXYZ accel_{0.f, 0.f, 1.f};   // device flat
  GyroXYZ  gyro_{0.f, 0.f, 0.f};
};

}  // namespace yui

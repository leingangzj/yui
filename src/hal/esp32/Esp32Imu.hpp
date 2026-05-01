#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/IImu.hpp"
#include <M5Unified.h>

namespace yui {

class Esp32Imu : public IImu {
public:
  bool init() override {
    return M5.Imu.begin();
  }
  bool read_accel(AccelXYZ& out) override {
    return M5.Imu.getAccelData(&out.x, &out.y, &out.z);
  }
  bool read_gyro(GyroXYZ& out) override {
    return M5.Imu.getGyroData(&out.x, &out.y, &out.z);
  }
};

}  // namespace yui
#endif

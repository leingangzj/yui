#pragma once

namespace yui {

struct AccelXYZ { float x, y, z; };  // g (gravity units)
struct GyroXYZ  { float x, y, z; };  // deg/sec

class IImu {
public:
  virtual ~IImu() = default;
  virtual bool init() = 0;
  virtual bool read_accel(AccelXYZ& out) = 0;
  virtual bool read_gyro(GyroXYZ& out)   = 0;
};

}  // namespace yui

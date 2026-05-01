#pragma once
#include <cstdint>
#include <cstddef>

namespace yui {

class IIr {
public:
  virtual ~IIr() = default;
  virtual bool init() = 0;
  // Send NEC-format code on `carrier_khz` (typ. 38). addr/cmd are NEC fields.
  virtual bool send_nec(uint16_t addr, uint16_t cmd, uint16_t carrier_khz = 38) = 0;
  // Number of times send_nec has been invoked (test convenience).
  virtual uint32_t sent_count() const = 0;
};

}  // namespace yui

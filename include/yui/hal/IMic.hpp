#pragma once
#include <cstdint>
#include <cstddef>

namespace yui {

class IMic {
public:
  virtual ~IMic() = default;
  virtual bool init() = 0;
  // Read up to `cap` int16 PCM samples; return count actually read.
  // Non-blocking: returns 0 if nothing ready.
  virtual size_t read_samples(int16_t* out, size_t cap) = 0;
};

}  // namespace yui

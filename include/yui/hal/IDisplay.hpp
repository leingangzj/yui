#pragma once
#include "yui/types.hpp"
#include <cstddef>
#include <cstdint>

namespace yui {

class IDisplay {
public:
  virtual ~IDisplay() = default;
  virtual int width() const  = 0;
  virtual int height() const = 0;
  virtual void clear(Color c) = 0;
  virtual void put_pixel(int x, int y, Color c) = 0;
  virtual void fill_rect(Rect r, Color c) = 0;
  virtual void draw_text(int x, int y, const char* s, Color fg, Color bg) = 0;

  // Decode + blit a PNG. Optional — backends without a PNG decoder ignore
  // the call. Used by the device boot animation; native tests hit the
  // default no-op so they don't need an image library.
  virtual void draw_png(const uint8_t* /*data*/, std::size_t /*len*/,
                        int /*x*/, int /*y*/) {}

  virtual void flush() = 0;
};

}  // namespace yui

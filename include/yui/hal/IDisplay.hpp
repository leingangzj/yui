#pragma once
#include "yui/types.hpp"

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
  virtual void flush() = 0;
};

}  // namespace yui

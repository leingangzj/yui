#pragma once
#include "yui/hal/IDisplay.hpp"
#include <vector>
#include <string>
#include <cstring>

namespace yui {

// Off-screen framebuffer impl for host-side tests.
class NativeDisplay : public IDisplay {
public:
  NativeDisplay(int w = kScreenWidth, int h = kScreenHeight)
    : w_(w), h_(h), buf_(w * h, kBlack) {}

  int width() const override  { return w_; }
  int height() const override { return h_; }

  void clear(Color c) override {
    std::fill(buf_.begin(), buf_.end(), c);
  }

  void put_pixel(int x, int y, Color c) override {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return;
    buf_[y * w_ + x] = c;
  }

  void fill_rect(Rect r, Color c) override {
    for (int y = r.y; y < r.y + r.h; ++y)
      for (int x = r.x; x < r.x + r.w; ++x)
        put_pixel(x, y, c);
  }

  void draw_text(int x, int y, const char* s, Color /*fg*/, Color /*bg*/) override {
    last_text_ = s ? s : "";
    last_text_x_ = x;
    last_text_y_ = y;
  }

  void flush() override { ++flush_count_; }

  // Test inspection
  Color pixel_at(int x, int y) const {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return kBlack;
    return buf_[y * w_ + x];
  }
  const std::string& last_text() const { return last_text_; }
  int flush_count() const { return flush_count_; }

private:
  int w_, h_;
  std::vector<Color> buf_;
  std::string last_text_;
  int last_text_x_ = 0, last_text_y_ = 0;
  int flush_count_ = 0;
};

}  // namespace yui

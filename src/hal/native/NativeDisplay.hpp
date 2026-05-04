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

  void draw_text_styled(int x, int y, const char* s,
                        Color /*fg*/, Color /*bg*/,
                        FontStyle /*style*/) override {
    last_text_ = s ? s : "";
    last_text_x_ = x;
    last_text_y_ = y;
    if (s && *s) {
      if (!all_text_.empty()) all_text_ += '\n';
      all_text_ += s;
    }
  }

  int text_width(const char* s, FontStyle style) override {
    int n = 0;
    while (s && *s++) ++n;
    return n * cell_w_(style);
  }

  int line_height(FontStyle style) override {
    return cell_h_(style);
  }

  void flush() override {
    ++flush_count_;
    // Each flush starts a fresh accumulation window — tests that drive a
    // full render() then assert on all_text() see exactly that frame's
    // text without contamination from earlier setup renders.
    all_text_for_last_frame_ = all_text_;
    all_text_.clear();
  }

  // Test inspection
  Color pixel_at(int x, int y) const {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return kBlack;
    return buf_[y * w_ + x];
  }
  const std::string& last_text() const { return last_text_; }
  // All text drawn during the most-recently-completed frame, joined by \n.
  const std::string& all_text() const { return all_text_for_last_frame_; }
  int flush_count() const { return flush_count_; }

private:
  // Native cell metrics — fixed grid so tests stay deterministic. Title
  // is 2× the body cell to match the device's relative sizing.
  static int cell_w_(FontStyle s) {
    return (s == FontStyle::Title) ? 12 : 6;
  }
  static int cell_h_(FontStyle s) {
    return (s == FontStyle::Title) ? 16 : 8;
  }

  int w_, h_;
  std::vector<Color> buf_;
  std::string last_text_;
  std::string all_text_;
  std::string all_text_for_last_frame_;
  int last_text_x_ = 0, last_text_y_ = 0;
  int flush_count_ = 0;
};

}  // namespace yui

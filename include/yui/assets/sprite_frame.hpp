#pragma once
#include <cstddef>
#include <cstdint>

namespace yui::assets {

struct SpriteFrame {
  const uint8_t* data;
  std::size_t    len;
};

}  // namespace yui::assets

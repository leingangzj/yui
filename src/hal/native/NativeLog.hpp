#pragma once
#include "yui/hal/ILog.hpp"
#include <cstdio>

namespace yui {

class StderrLog : public ILog {
public:
  void info(const char* msg)  override { std::fprintf(stderr, "[info]  %s\n", msg); }
  void warn(const char* msg)  override { std::fprintf(stderr, "[warn]  %s\n", msg); }
  void error(const char* msg) override { std::fprintf(stderr, "[error] %s\n", msg); }
};

}  // namespace yui

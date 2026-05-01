#pragma once

namespace yui {

class ILog {
public:
  virtual ~ILog() = default;
  virtual void info(const char* msg)  = 0;
  virtual void warn(const char* msg)  = 0;
  virtual void error(const char* msg) = 0;
};

}  // namespace yui

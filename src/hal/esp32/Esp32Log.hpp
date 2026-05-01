#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

#include "yui/hal/ILog.hpp"
#include <Arduino.h>

namespace yui {

class SerialLog : public ILog {
public:
  void info(const char* msg)  override { Serial.print("[info]  "); Serial.println(msg); }
  void warn(const char* msg)  override { Serial.print("[warn]  "); Serial.println(msg); }
  void error(const char* msg) override { Serial.print("[error] "); Serial.println(msg); }
};

}  // namespace yui
#endif

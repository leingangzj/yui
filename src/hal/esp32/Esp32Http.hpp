#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32Http — wraps Arduino's HTTPClient + WiFiClient for the
// Pineapple REST API. v0.2 Track B code.
//
// STUB: returns -1 (transport error) until the real impl lands.
#include "yui/hal/IHttp.hpp"

namespace yui {

class Esp32Http : public IHttp {
public:
  int request(const char* /*method*/, const char* /*url*/,
              const char* /*request_body*/, const char* /*auth_header*/,
              char* out_body, size_t out_cap,
              uint32_t /*timeout_ms*/) override {
    if (out_cap > 0 && out_body) out_body[0] = '\0';
    return -1;
  }
};

}  // namespace yui
#endif

#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32Http — wraps Arduino's HTTPClient + WiFiClient. Used by
// Pineapple REST + (future) CelesTrak TLE fetch.
//
// Plain HTTP only — Pineapple speaks plaintext, and TLE fetch can
// use the http:// mirror. No TLS in v0.3 to keep the flash budget
// down (mbedtls is +200 KB).
#include "yui/hal/IHttp.hpp"
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <cstring>

namespace yui {

class Esp32Http : public IHttp {
public:
  int request(const char* method, const char* url,
              const char* request_body, const char* auth_header,
              char* out_body, size_t out_cap,
              uint32_t timeout_ms) override {
    if (!url || !method) return -1;
    if (out_body && out_cap > 0) out_body[0] = '\0';

    HTTPClient http;
    http.setTimeout(timeout_ms);
    http.setReuse(false);
    if (!http.begin(url)) return -1;
    if (auth_header && *auth_header) {
      http.addHeader("Authorization", auth_header);
    }
    http.addHeader("Content-Type", "application/json");

    int code = -1;
    if (std::strcmp(method, "GET") == 0) {
      code = http.GET();
    } else if (std::strcmp(method, "POST") == 0) {
      code = http.POST(request_body ? request_body : "");
    } else if (std::strcmp(method, "PUT") == 0) {
      code = http.PUT(request_body ? request_body : "");
    } else if (std::strcmp(method, "DELETE") == 0) {
      code = http.sendRequest("DELETE", request_body ? request_body : "");
    } else {
      http.end();
      return -1;
    }

    if (code > 0 && out_body && out_cap > 0) {
      const String body = http.getString();
      const size_t n = (body.length() < out_cap - 1) ? body.length() : out_cap - 1;
      std::memcpy(out_body, body.c_str(), n);
      out_body[n] = '\0';
    }
    http.end();
    return code;
  }
};

}  // namespace yui
#endif

#pragma once
// IHttp — minimal HTTP client interface. v0.2 backs it with WiFiClient
// + HTTPClient on the ESP32; native tests use a FakeHttp that registers
// canned (method,url) → response pairs. Body is a plain UTF-8 byte
// buffer; the caller is responsible for JSON encode/decode (we use
// cJSON, already pulled in by ESP-IDF).
//
// This is intentionally tiny — no streaming, no chunking, no TLS.
// The Pineapple speaks plain HTTP, which is the only target in v0.2.
#include <cstdint>
#include <cstddef>

namespace yui {

class IHttp {
public:
  virtual ~IHttp() = default;

  // Returns the HTTP status code (e.g. 200), or -1 on transport error.
  // Body is written to out_body as a NUL-terminated string truncated to
  // out_cap-1 bytes. method is "GET" / "PUT" / "POST" / "DELETE".
  // request_body and auth_header may be null.
  virtual int request(const char* method,
                      const char* url,
                      const char* request_body,
                      const char* auth_header,
                      char*  out_body,
                      size_t out_cap,
                      uint32_t timeout_ms) = 0;
};

}  // namespace yui

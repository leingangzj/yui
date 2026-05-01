#pragma once
#include "yui/hal/IHttp.hpp"
#include <cstring>
#include <map>
#include <string>

namespace yui {

class FakeHttp : public IHttp {
public:
  struct Response {
    int         status = 200;
    std::string body;
  };

  // Register a canned response keyed by "METHOD URL".
  void register_response(const char* method, const char* url,
                         int status, const char* body) {
    Response r;
    r.status = status;
    r.body   = body ? body : "";
    responses_[key_(method, url)] = std::move(r);
  }

  int request(const char* method, const char* url,
              const char* request_body, const char* auth_header,
              char* out_body, size_t out_cap,
              uint32_t /*timeout_ms*/) override {
    last_method_ = method ? method : "";
    last_url_    = url ? url : "";
    last_body_   = request_body ? request_body : "";
    last_auth_   = auth_header ? auth_header : "";
    ++calls_;
    auto it = responses_.find(key_(method, url));
    if (it == responses_.end()) {
      if (out_cap > 0) out_body[0] = '\0';
      return -1;
    }
    if (out_cap > 0) {
      const size_t n = std::min(out_cap - 1, it->second.body.size());
      std::memcpy(out_body, it->second.body.data(), n);
      out_body[n] = '\0';
    }
    return it->second.status;
  }

  // Test inspection
  int          calls()       const { return calls_; }
  const std::string& last_method() const { return last_method_; }
  const std::string& last_url()    const { return last_url_; }
  const std::string& last_body()   const { return last_body_; }
  const std::string& last_auth()   const { return last_auth_; }

private:
  static std::string key_(const char* m, const char* u) {
    std::string k = m ? m : "";
    k += ' ';
    k += u ? u : "";
    return k;
  }

  std::map<std::string, Response> responses_;
  std::string last_method_, last_url_, last_body_, last_auth_;
  int calls_ = 0;
};

}  // namespace yui

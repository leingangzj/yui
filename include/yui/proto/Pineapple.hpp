#pragma once
// Pineapple REST client. See docs/protocols/PINEAPPLE_API.md.
//
// Uses an injected IHttp for transport so the client is fully
// exercisable in native tests via FakeHttp + canned JSON responses.
// Token is held in-memory; on a 401 we re-login transparently using
// the credentials passed to login() (also persisted via IStorage if
// provided).
#include "yui/hal/IHttp.hpp"
#include "yui/hal/IStorage.hpp"
#include "yui/util/JsonValue.hpp"
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace yui::pineapple {

struct Cards {
  int  cpu_pct           = 0;
  int  mem_pct           = 0;
  int  temp_c            = 0;
  int  clients_connected = 0;
  int  total_ssids       = 0;
};

struct ApInfo {
  char    ssid[33]      = {0};
  char    bssid[18]     = {0};
  char    encryption[16] = {0};
  uint8_t channel       = 0;
  int8_t  rssi          = 0;
};

class Client {
public:
  Client(IHttp& http, IStorage* store = nullptr) : http_(http), store_(store) {}

  bool authenticated() const { return token_[0] != 0; }
  uint16_t port() const { return port_; }
  const char* host() const { return host_; }

  // POST /api/login with {username, password}; on success copies the
  // token out of the response and persists creds if store_ is set.
  bool login(const char* host, uint16_t port,
             const char* user, const char* pass) {
    if (!host || !user || !pass) return false;
    std::strncpy(host_, host, sizeof(host_) - 1);
    host_[sizeof(host_) - 1] = '\0';
    port_ = port;
    std::strncpy(user_, user, sizeof(user_) - 1);
    user_[sizeof(user_) - 1] = '\0';
    std::strncpy(pass_, pass, sizeof(pass_) - 1);
    pass_[sizeof(pass_) - 1] = '\0';

    char url[80];
    std::snprintf(url, sizeof(url), "http://%s:%u/api/login", host_,
                  static_cast<unsigned>(port_));
    char body[160];
    std::snprintf(body, sizeof(body),
                  "{\"username\":\"%s\",\"password\":\"%s\"}", user_, pass_);
    char resp[640];
    const int rc = http_.request("POST", url, body, nullptr,
                                  resp, sizeof(resp), 5000);
    if (rc != 200) return false;
    if (!json::find_string(resp, "token", token_, sizeof(token_))) return false;

    if (store_) {
      store_->put_str("pa.host", host_);
      store_->put_str("pa.user", user_);
      store_->put_str("pa.pass", pass_);
      store_->put_int("pa.port", static_cast<int32_t>(port_));
    }
    return true;
  }

  bool dashboard_cards(Cards& out) {
    char resp[1024];
    if (!authed_get_("/api/dashboard/cards", resp, sizeof(resp))) return false;
    json::find_int(resp, "cpuUsage",         &out.cpu_pct);
    json::find_int(resp, "memoryUsage",      &out.mem_pct);
    json::find_int(resp, "temperature",      &out.temp_c);
    json::find_int(resp, "clientsConnected", &out.clients_connected);
    json::find_int(resp, "totalSSIDs",       &out.total_ssids);
    return true;
  }

  bool recon_start(bool live, int scan_seconds, const char* band, int& scan_id) {
    char body[96];
    std::snprintf(body, sizeof(body),
                  "{\"live\":%s,\"scan_time\":%d,\"band\":\"%s\"}",
                  live ? "true" : "false", scan_seconds,
                  band ? band : "2.4ghz");
    char resp[256];
    if (!authed_request_("POST", "/api/recon/start", body,
                         resp, sizeof(resp))) return false;
    int sid = 0;
    if (!json::find_int(resp, "scanID", &sid)) return false;
    scan_id = sid;
    return true;
  }

  bool recon_status(int& scan_id_out, bool& running, int& percent) {
    char resp[256];
    if (!authed_get_("/api/recon/status", resp, sizeof(resp))) return false;
    json::find_int(resp,  "scanID",      &scan_id_out);
    json::find_bool(resp, "scanRunning", &running);
    json::find_int(resp,  "scanPercent", &percent);
    return true;
  }

  // STUB until v0.2-stretch: parsing the APResults array needs array
  // iteration which JsonValue doesn't do. PineappleReconApp will get
  // the count via dashboard_cards and the full result via a follow-up
  // commit that adds an array helper to JsonValue.
  bool recon_results(int /*scan_id*/, ApInfo* /*out_aps*/, size_t /*cap*/,
                     size_t& count) { count = 0; return false; }

private:
  bool authed_get_(const char* path, char* resp, size_t cap) {
    return authed_request_("GET", path, nullptr, resp, cap);
  }

  bool authed_request_(const char* method, const char* path,
                       const char* body, char* resp, size_t cap) {
    if (!authenticated()) return false;
    char url[128];
    std::snprintf(url, sizeof(url), "http://%s:%u%s", host_,
                  static_cast<unsigned>(port_), path);
    char auth[280];
    std::snprintf(auth, sizeof(auth), "Bearer %s", token_);
    int rc = http_.request(method, url, body, auth, resp, cap, 5000);
    if (rc == 401) {
      // Token expired — relogin once and retry.
      token_[0] = '\0';
      if (!login(host_, port_, user_, pass_)) return false;
      std::snprintf(auth, sizeof(auth), "Bearer %s", token_);
      rc = http_.request(method, url, body, auth, resp, cap, 5000);
    }
    return rc >= 200 && rc < 300;
  }

  IHttp&     http_;
  IStorage*  store_  = nullptr;
  uint16_t   port_   = 1471;
  char       host_[64]   = {0};
  char       user_[40]   = {0};
  char       pass_[40]   = {0};
  char       token_[256] = {0};
};

}  // namespace yui::pineapple

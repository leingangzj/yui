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

    char url[128];
    std::snprintf(url, sizeof(url), "http://%.63s:%u/api/login", host_,
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

  // Parses APResults array from /api/recon/scans/<id>. Each element
  // is a small flat object — we walk the response substring-style,
  // extracting one record at a time via JsonValue. Caps at `cap`.
  bool recon_results(int scan_id, ApInfo* out_aps, size_t cap, size_t& count) {
    count = 0;
    char path[64];
    std::snprintf(path, sizeof(path), "/api/recon/scans/%d", scan_id);
    char resp[4096];
    if (!authed_get_(path, resp, sizeof(resp))) return false;
    // Find APResults array. Then iterate "{...}" objects within it.
    const char* p = std::strstr(resp, "\"APResults\"");
    if (!p) return true;          // no APs but request succeeded
    p = std::strchr(p, '[');
    if (!p) return true;
    ++p;
    while (count < cap) {
      const char* obj = std::strchr(p, '{');
      if (!obj) break;
      const char* end = std::strchr(obj, '}');
      if (!end) break;
      const size_t olen = static_cast<size_t>(end - obj + 1);
      char one[512];
      const size_t copy = (olen < sizeof(one) - 1) ? olen : (sizeof(one) - 1);
      std::memcpy(one, obj, copy);
      one[copy] = '\0';
      ApInfo& ap = out_aps[count];
      json::find_string(one, "ssid",       ap.ssid,       sizeof(ap.ssid));
      json::find_string(one, "bssid",      ap.bssid,      sizeof(ap.bssid));
      json::find_string(one, "encryption", ap.encryption, sizeof(ap.encryption));
      int ch = 0, rs = 0;
      if (json::find_int(one, "channel", &ch)) ap.channel = static_cast<uint8_t>(ch);
      if (json::find_int(one, "rssi",    &rs)) ap.rssi    = static_cast<int8_t>(rs);
      ++count;
      p = end + 1;
    }
    return true;
  }

  // ─── PineAP settings (Evil Twin / Karma) ──────────────────────────
  bool pineap_set_enabled(bool enabled, bool karma) {
    char body[160];
    std::snprintf(body, sizeof(body),
                  "{\"enablePineAP\":%s,\"karma\":%s}",
                  enabled ? "true" : "false",
                  karma   ? "true" : "false");
    char resp[128];
    return authed_request_("PUT", "/api/pineap/settings", body,
                           resp, sizeof(resp));
  }

  bool pineap_add_ssid(const char* ssid) {
    char body[80];
    std::snprintf(body, sizeof(body), "{\"ssid\":\"%s\"}", ssid ? ssid : "");
    char resp[128];
    return authed_request_("PUT", "/api/pineap/ssids/ssid", body,
                           resp, sizeof(resp));
  }

  bool pineap_clear_ssids() {
    char resp[128];
    return authed_request_("DELETE", "/api/pineap/ssids", nullptr,
                           resp, sizeof(resp));
  }

  // ─── Handshake browser endpoints ──────────────────────────────────
  // Returns the raw JSON body of /api/pineap/handshakes for the caller
  // to render. Avoids parsing the (per-Pineapple-version-variable)
  // shape; PineappleHandshakeApp counts entries by scanning for "bssid".
  bool handshakes_raw(char* out, size_t cap) {
    return authed_get_("/api/pineap/handshakes", out, cap);
  }

  // ─── Deauth (passes through to Pineapple — we never TX directly) ──
  bool deauth_ap(const char* bssid, uint8_t channel, int multiplier = 1) {
    char body[128];
    std::snprintf(body, sizeof(body),
        "{\"bssid\":\"%s\",\"channel\":%u,\"multiplier\":%d,\"clients\":[]}",
        bssid ? bssid : "", static_cast<unsigned>(channel), multiplier);
    char resp[128];
    return authed_request_("POST", "/api/pineap/deauth/ap", body,
                           resp, sizeof(resp));
  }

  bool deauth_client(const char* bssid, const char* mac, uint8_t channel,
                     int multiplier = 1) {
    char body[160];
    std::snprintf(body, sizeof(body),
        "{\"bssid\":\"%s\",\"mac\":\"%s\",\"channel\":%u,\"multiplier\":%d}",
        bssid ? bssid : "", mac ? mac : "",
        static_cast<unsigned>(channel), multiplier);
    char resp[128];
    return authed_request_("POST", "/api/pineap/deauth/client", body,
                           resp, sizeof(resp));
  }

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

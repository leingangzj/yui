#pragma once
// Pineapple REST client. See docs/protocols/PINEAPPLE_API.md.
//
// STATUS: types final, methods stubbed (return false). Implementation
// lands in v0.2 Track B against an injected IHttp.
#include "yui/hal/IHttp.hpp"
#include "yui/hal/IStorage.hpp"
#include <cstdint>
#include <cstddef>

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
  char    encryption[16] = {0};   // "wpa2", "open", ...
  uint8_t channel       = 0;
  int8_t  rssi          = 0;
};

class Client {
public:
  Client(IHttp& http, IStorage* store = nullptr) : http_(http), store_(store) {}

  // Saves host/port/user/pass to NVS (when store provided), then logs in.
  // Sets internal bearer token on success. STUB returns false.
  bool login(const char* /*host*/, uint16_t /*port*/,
             const char* /*user*/, const char* /*pass*/) { return false; }

  // Returns true if we currently hold a non-expired token.
  bool authenticated() const { return token_[0] != 0; }

  // Endpoints (v0.2 must-haves) — all STUBBED until Track B impl.
  bool dashboard_cards(Cards& /*out*/) { return false; }
  bool recon_start(bool /*live*/, int /*scan_seconds*/,
                   const char* /*band*/, int& /*scan_id*/) { return false; }
  bool recon_status(int& /*scan_id_out*/, bool& /*running*/,
                    int& /*percent*/) { return false; }
  bool recon_results(int /*scan_id*/, ApInfo* /*out_aps*/, size_t /*cap*/,
                     size_t& /*count*/) { return false; }

private:
  IHttp&     http_;
  IStorage*  store_  = nullptr;
  uint16_t   port_   = 1471;
  char       host_[40] = {0};
  char       token_[256] = {0};
};

}  // namespace yui::pineapple

# WiFi Pineapple Mark VII — REST API

**Date:** 2026-05-01
**Sources:**

- Official: `hak5.github.io/mk7-docs/docs/rest/...`
- Community: `TW-D/WiFi-Pineapple-MK7_REST-Client` (Python reference impl)

---

## Connection model

The Cardputer connects to the Pineapple's WiFi network as a normal STA, then makes plain HTTP REST calls. **No SSL** by default — the Pineapple's web UI runs on plain HTTP at port `1471`.

```
Cardputer (WiFi STA) ─── HTTP/JSON ───▶ Pineapple :1471
```

| Item                    | Value                                  |
| ----------------------- | -------------------------------------- |
| Default Pineapple AP IP | `172.16.42.1`                          |
| API base                | `http://172.16.42.1:1471/api`          |
| Auth scheme             | Bearer token in `Authorization` header |
| TLS                     | None                                   |
| JSON encoding           | UTF-8                                  |

---

## Auth flow

### 1. Login (one-shot, then cache token)

```
POST /api/login
Content-Type: application/json

{ "username": "root", "password": "<user-set>" }
```

Response:

```json
{ "token": "eyJVc2VyIjoicm9vdCIsIkV4cGlyeSI6...=.<sig>=" }
```

Token is **base64-encoded JSON with embedded expiry** + an HMAC signature (custom format, not standard JWT). The expiry is in the decoded prefix.

### 2. Subsequent calls

```
Authorization: Bearer <token>
```

### 3. Token lifetime

Embedded in the token's payload (`Expiry` field). **No refresh endpoint** — when the token expires, log in again. For Yui this means: store user/pass in NVS, auto-relogin on 401.

### 4. Errors

`{ "error": "...message..." }` with HTTP 4xx / 5xx.

---

## Endpoints we need for v0.2 (must-haves)

### PineappleApp — status

```
GET /api/dashboard/cards
```

Returns:

```json
{
  "systemStatus": { "cpuUsage": 0..100, "memoryUsage": 0..100, "temperature": <°C> },
  "diskUsage": { "rootUsage": "..." },
  "clientsConnected": "<count as string>",
  "previousClients": "<count>",
  "ssidsSeen": { "totalSSIDs": "<n>", "currentSSIDs": "<n>" }
}
```

Renders nicely as a 5-row screen: **CPU% • Mem% • Temp • Clients • SSIDs**.

### PineappleReconApp — paged AP/client list

```
POST /api/recon/start
Body: { "live": true, "scan_time": 30, "band": "2.4ghz" }
→ { "scanRunning": true, "scanID": 42 }
```

Then poll:

```
GET /api/recon/status
→ { "scanRunning": true, "scanPercent": 0..100, "scanID": 42 }
```

When `scanRunning` flips to `false`:

```
GET /api/recon/scans/42
→ { "APResults": [ { "ssid", "bssid", "encryption", "channel", "rssi", "clients": [...] }, ... ] }
```

Render paged list 7 rows per screen (matches our launcher pattern).

`band` accepts `"2.4ghz"`, `"5ghz"`, or `"both"`.

---

## Endpoints we need for v0.2 stretch / v0.3

### HandshakeApp — list captured handshakes

```
GET /api/pineap/handshakes
→ { "handshakes": [ { ... }, ... ] }

POST /api/pineap/handshakes/start
Body: { "bssid": "AA:BB:CC:DD:EE:FF", "channel": 6 }

POST /api/pineap/handshakes/stop

GET /api/pineap/handshakes/check
→ { "captureRunning": bool, "bssid": string }

DELETE /api/pineap/handshakes/delete
Body: { "type": "wpa", "bssid": "..." }
```

### EvilTwinApp — PineAP settings

```
GET  /api/pineap/settings
PUT  /api/pineap/settings
Body: full PineAPSettings object (enablePineAP, autostartPineAP, ap_channel, beacon_response_*, karma, logging, MAC pool, ...)

GET  /api/pineap/ssids                  → newline-separated SSIDs
PUT  /api/pineap/ssids/ssid             Body: { "ssid": "..." }
DELETE /api/pineap/ssids/ssid           Body: { "ssid": "..." }
DELETE /api/pineap/ssids                clears pool
```

### KarmaApp — connected clients

```
GET /api/pineap/clients               → Client[]
GET /api/pineap/clients/count         → number
DELETE /api/pineap/clients/kick       Body: { "mac": "..." }
GET /api/pineap/previousclients       → PreviousClient[]
```

### Deauth (gated behind "I own this network" confirm)

```
POST /api/pineap/deauth/ap
Body: { "bssid": "...", "multiplier": 1, "channel": 6, "clients": ["..."] }

POST /api/pineap/deauth/client
Body: { "bssid": "...", "mac": "...", "multiplier": 1, "channel": 6 }
```

---

## ESP32 implementation skeleton

### HAL: `IHttp`

```cpp
// include/yui/hal/IHttp.hpp
class IHttp {
public:
  virtual ~IHttp() = default;

  // Returns -1 on error; otherwise HTTP status code. Body written to out_body.
  virtual int request(const char* method,
                      const char* url,
                      const char* body,         // may be null
                      const char* auth_header,  // may be null
                      char* out_body,
                      size_t out_cap) = 0;
};
```

`Esp32Http` wraps `WiFiClient` + `HTTPClient`. `FakeHttp` lets tests register canned response per (method, url) tuple.

### Pineapple client wrapper

```cpp
// include/yui/proto/Pineapple.hpp
class PineappleClient {
public:
  PineappleClient(IHttp& http, IStorage* store = nullptr);

  bool login(const char* host, uint16_t port,
             const char* user, const char* pass);

  // Status
  struct DashboardCards {
    int cpu_pct; int mem_pct; int temp_c;
    int clients_connected; int total_ssids;
  };
  bool dashboard_cards(DashboardCards& out);

  // Recon
  bool recon_start(bool live, int scan_seconds, const char* band, int& scan_id);
  bool recon_status(int& scan_id_out, bool& running, int& percent);
  bool recon_results(int scan_id, ApInfo* out_aps, size_t cap, size_t& count);
  // ...
};
```

### Persistence (NVS keys)

| Key       | Value                                                           | Source     |
| --------- | --------------------------------------------------------------- | ---------- |
| `pa.host` | host string e.g. `172.16.42.1`                                  | user setup |
| `pa.port` | uint16_t (default `1471`)                                       | user setup |
| `pa.user` | username                                                        | user setup |
| `pa.pass` | password                                                        | user setup |
| `pa.ssid` | the Pineapple's WiFi SSID (so we know which AP to associate to) | user setup |
| `pa.psk`  | the Pineapple's WiFi password                                   | user setup |

When `PineappleApp` opens, if `pa.ssid` is set, instruct `INet` to associate to that SSID (overriding `wifi.ssid`) for the duration of the app — and revert on exit. This is critical because the user's normal WiFi and the Pineapple are different networks.

### JSON parsing

ESP-IDF includes `cJSON` — already linked. Sufficient for our shapes (small flat objects + small arrays). Don't pull in ArduinoJson; cJSON is leaner.

---

## Quirks & gotchas

1. **Pineapple's WiFi cannot run alongside the user's WiFi from a single STA.** The Cardputer must associate **to the Pineapple AP** to talk to it — which means losing the home WiFi connection. Our UX: PineappleApp's first action on enter is "switching networks…". On exit, restore previous AP. Surface this clearly.
2. **No TLS.** Anyone on the same Pineapple AP can sniff the bearer token. This is fine for our use case (you control the AP) but worth a comment in the code.
3. **Token doesn't auto-refresh.** Wrap every call in "if 401 → re-login → retry once." Cap the retry depth to avoid login loops.
4. **`Authorization: Bearer <token>` is the only auth method**. No basic-auth fallback, no API key.
5. **Some endpoints take their body field as `"mac"` even when semantically a SSID** (the docs note this on the SSID-filter endpoint). Read the field name for each endpoint, don't assume.
6. **`/api/recon/start` is async**; you must poll `/api/recon/status` until `scanRunning` is `false`. Don't block the UI thread for 30 seconds — poll from the app's `tick()`.
7. **MTU**: Cardputer's `WiFiClient` reads in ~1.4 KB chunks. Recon results for a busy area can run multi-KB. Stream-parse, don't buffer the whole thing.

---

## Test approach (native)

`FakeHttp` registers expected (method, URL) → canned JSON response pairs. Then the `PineappleClient` is exercised against it deterministically. No real network involved.

Sample tests:

```cpp
TEST(PineappleClient, login_stores_token)
TEST(PineappleClient, dashboard_cards_parses_status)
TEST(PineappleClient, recon_start_then_status_then_results)
TEST(PineappleClient, 401_triggers_relogin_and_retries)
TEST(PineappleClient, malformed_json_returns_false_no_crash)
```

---

## Sources

- [WiFi Pineapple Mark 7 REST docs (root)](https://hak5.github.io/mk7-docs/docs/rest/rest/)
- [Authentication](https://hak5.github.io/mk7-docs/docs/rest/authentication/authentication/)
- [Dashboard](https://hak5.github.io/mk7-docs/docs/rest/dashboard/dashboard/)
- [Recon](https://hak5.github.io/mk7-docs/docs/rest/recon/recon/)
- [PineAP](https://hak5.github.io/mk7-docs/docs/rest/pineap/pineap/)
- [TW-D/WiFi-Pineapple-MK7_REST-Client (Python reference)](https://github.com/TW-D/WiFi-Pineapple-MK7_REST-Client)

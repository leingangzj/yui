# Kenwood TH-D75 — Bluetooth + Command Protocol

**Date:** 2026-05-01
**Sources:**

- Official "TH-D75A/E Operating Tips" PDF (Kenwood, sec. 2.7 KISS TNC)
- `islandmagic/bb-link` repo on GitHub — production code that bridges TH-D75 BT-Classic ↔ iOS BLE
- `LA3QMA/TH-D74-Kenwood` wiki — community-maintained 51-command reference (TH-D74 protocol; TH-D75 is a superset)

---

## 🚨 Critical correction to v0.2 plan

**The TH-D75 uses Bluetooth Classic (RFCOMM/SPP), not BLE.**

This was an incorrect assumption in `yui-v0.2-scope.md`. Implications:

- Our HAL needs to be `IRadioLink` over `BluetoothSerial` (Bluedroid Classic stack), **not** NimBLE.
- ESP32-S3 supports BT Classic + WiFi co-existence, but the Bluedroid stack must be enabled in `sdkconfig` — Arduino-IDF's default does include it via `BluetoothSerial.h`.
- BT Classic costs ~50 KB more SRAM than BLE-only. We have ~243 KB free → still fine.
- Pairing is PIN-based (default `"0000"`), not BLE Just-Works.
- Flash impact: Bluedroid is heavier than NimBLE; expect another ~150 KB flash. We're at 1.59 MB / 6.4 MB partition → still fine.

**Action:** update `yui-v0.2-scope.md` "IRadio (BLE-serial backend)" → "IRadioLink (BT-Classic SPP)".

---

## Bluetooth pairing & connection (verified from `bb-link` source)

### Discovery

- Filter by **Class of Device** = `0x620204` (Kenwood handhelds). This is reliable — no need to match device-name strings.
- ESP32 BluetoothSerial can scan in master role: `btSerial.discoverAsync(...)` or use `connect(addr, channel, sec, role)` directly when MAC is known.
- **Quirk:** the Arduino BluetoothSerial library can only scan **before** any connection; once connected, you must reboot to scan again. (We can store a paired MAC in NVS — already shipped key `wifi.ssid`-style, just add `radio.mac`.)

### Pairing

1. User: Menu → Bluetooth → Pairing Mode on the radio.
2. Cardputer: `BluetoothSerial::setPin("0000")` (default; or read from radio's PIN dialog).
3. Cardputer: `btSerial.connect(mac, 0, ESP_SPP_SEC_NONE, ESP_SPP_ROLE_MASTER)`.
4. Radio prompts user to confirm; user presses OK.
5. Connection up — store MAC in NVS for auto-reconnect.

### Auto-reconnect

- After first pair, `btSerial.connect(mac, 0, ESP_SPP_SEC_NONE, ESP_SPP_ROLE_MASTER)` reconnects directly without re-pairing.

### Serial parameters once connected

- 9600 8-N-1 (per Kenwood Operating Tips §2.8)
- ASCII command/response, all commands `\r`-terminated
- Response validation: first 2 chars of response echo the command code

---

## Two operating modes on the same SPP link

### Mode 1: Command/Control (default)

- ASCII two-letter commands (51 total in TH-D74 set; TH-D75 is a superset).
- Used for VFO frequency, mode, TNC enable/disable, identification, etc.
- Send `BT\r` — if it returns `BT 0\r` or `BT 1\r`, you're in command mode.

### Mode 2: KISS TNC (after `TN 2,0\r`)

- Pure binary KISS frames, AX.25 payloads inside.
- All ASCII commands stop working; only KISS escape exits.
- Send `0xC0 0xFF 0xC0` (KISS Return frame) to exit back to command mode.
- Detection: send `BT\r`; no valid response → in KISS mode.

---

## Commands needed for v0.2 Track A (essentials)

| Code | Direction | Purpose                                      | Syntax                                                                                                                |
| ---- | --------- | -------------------------------------------- | --------------------------------------------------------------------------------------------------------------------- |
| `ID` | Q         | Get radio model (verify it's TH-D75)         | `ID\r` → `ID TH-D75A\r`                                                                                               |
| `TY` | Q         | Get radio type/firmware                      | `TY\r`                                                                                                                |
| `BT` | Q         | Bluetooth status; doubles as KISS-mode probe | `BT\r` → `BT n\r` (or no response in KISS)                                                                            |
| `AS` | Q/S       | Serial baud rate                             | `AS\r`, `AS n\r` (n: 0=1200, 1=9600)                                                                                  |
| `TN` | Q/S       | TNC mode                                     | `TN\r` → `TN m,v\r`<br>`TN 0,0\r` = TNC off<br>`TN 1,0\r` = APRS mode<br>`TN 2,0\r` = **KISS mode** (space mandatory) |
| `FQ` | Q/S       | VFO frequency                                | `FQ %d\r` (query VFO n)<br>`FQ %d,%010d\r` (set VFO n to N Hz)                                                        |
| `MD` | Q/S       | VFO mode (FM/DV/AM/SSB/CW…)                  | `MD %d\r`, `MD %d,%d\r`                                                                                               |

### KISS frame format (per AX.25 KISS spec)

```
FEND  CMD  PAYLOAD…  FEND
0xC0  TT   …         0xC0
```

Where `CMD` byte = `(port_nibble << 4) | command_nibble`:

| Cmd  | Name        | Payload                                             |
| ---- | ----------- | --------------------------------------------------- |
| 0x00 | Data        | AX.25 frame                                         |
| 0x01 | TXDELAY     | 1 byte (×10ms)                                      |
| 0x02 | P-persist   | 1 byte (0–255)                                      |
| 0x03 | SlotTime    | 1 byte (×10ms)                                      |
| 0x04 | TXtail      | 1 byte (×10ms)                                      |
| 0x05 | FullDuplex  | 1 byte (0=half, !0=full)                            |
| 0x06 | SetHardware | varies (TH-D75: 0x00/0x23 = 1200, 0x05/0x26 = 9600) |
| 0xFF | Return      | (none — exits KISS)                                 |

### KISS escaping inside payload

| Byte        | Replace with           |
| ----------- | ---------------------- |
| 0xC0 (FEND) | 0xDB 0xDC (FESC TFEND) |
| 0xDB (FESC) | 0xDB 0xDD (FESC TFESC) |

This is critical — failure to escape FEND inside the payload will desynchronize the radio's frame parser.

---

## Commands needed for v0.3 SatTracker

| Code        | Direction | Purpose                                                            | Syntax                       |
| ----------- | --------- | ------------------------------------------------------------------ | ---------------------------- |
| `FQ`        | S         | Doppler-corrected RX frequency, called every 1–2 sec during a pass | `FQ 0,%010d\r` (10-digit Hz) |
| `BC`        | Q/S       | Band control / band select                                         | `BC\r`, `BC n\r`             |
| `MD`        | S         | Switch mode if working an FM/SSB sat                               | `MD 0,0\r` (set VFO 0 to FM) |
| `TX` / `RX` | S         | PTT (only if we ever automate transmit)                            | `TX\r` / `RX\r`              |

### CAT command rate-limit observation

The radio's PLL retunes in ~30–80 ms. Issuing `FQ` faster than ~5 Hz risks dropped commands. SatTracker's Doppler loop should run at 2 Hz (every 500 ms) — well under any limit, comfortable for the 437 MHz 70 cm bird that needs ~±10 kHz/pass ≈ 5 Hz/sec drift.

---

## Commands of interest for later (v0.3+ features)

From the LA3QMA wiki — 51 commands total. Notable ones:

`AE AG AI AS BC BE BL BS BT BY CS DC DL DS DW FO FQ FR FS FT FV GM GP GS ID IO LC MD ME MR MS PC PS PT RA RT RX SF SH SM SQ SR TN TX TY UP VD VG VM VX`

For Yui specifically:

- **`GP`** — GPS data (we'll use the radio's onboard GNSS for `IGnss` HAL)
- **`AI`** — auto-info push (radio sends unsolicited state changes; useful for live VFO display in RemoteHeadApp)
- **`MR`** — memory recall (RemoteHeadApp memory editor)
- **`ME`** — memory edit
- **`SF`** — squelch frequency? (TBD)
- **`PC`** — PC mode? (likely already on once we open serial)

---

## ESP32 implementation skeleton (HAL we need)

```cpp
// include/yui/hal/IRadioLink.hpp
class IRadioLink {
public:
  virtual ~IRadioLink() = default;
  virtual bool connect(const char* mac, const char* pin = "0000") = 0;
  virtual bool connected() const = 0;
  virtual void disconnect() = 0;

  // Command/control mode
  virtual bool send_command(const char* cmd, char* response, size_t cap,
                            uint32_t timeout_ms = 1000) = 0;

  // KISS mode
  virtual bool enter_kiss(uint8_t vfo = 0) = 0;
  virtual bool exit_kiss() = 0;
  virtual int  kiss_read(uint8_t* buf, size_t cap) = 0;     // raw bytes
  virtual bool kiss_write(const uint8_t* frame, size_t len) = 0;
};

// src/hal/esp32/Esp32RadioLink.hpp wraps BluetoothSerial.
// src/hal/native/FakeRadioLink.hpp lets tests inject canned responses.
```

### Discovery flow on Cardputer

1. Boot: read `radio.mac` from NVS. If present → straight to `connect()`.
2. If absent or connect fails → enter pairing UI in `KenwoodApp`:
   - User puts radio in pairing mode.
   - Cardputer `discover_async()` filtering by COD `0x620204`.
   - List candidates with name + MAC; user picks.
   - `setPin("0000"); connect(mac);`
   - On success: persist MAC to NVS as `radio.mac`.
3. Future boots auto-reconnect.

---

## Open questions (for hardware bring-up)

1. Does `M5Unified`'s default `sdkconfig` enable Bluedroid Classic? (BluetoothSerial.h must compile + link in our build.)
2. What's the actual concurrent throughput when WiFi STA + BT-Classic SPP are both active? (Phase 0 stress test.)
3. Does the TH-D75 send unsolicited messages (e.g. RX squelch open) on the SPP link, or is it strictly request/response in command mode? (Affects whether we need a separate RX parser thread.)
4. Confirm the `TN 2,0` syntax — Kenwood doc explicitly says **the space is mandatory** between `TN` and `2`.
5. Does `AI 1\r` work to enable auto-info push? (Would simplify RemoteHeadApp.)

---

## Sources

- [TH-D75A/E Operating Tips (Kenwood official PDF)](https://www.kenwood.com/i/products/info/amateur/pdf/TH-D75AE_IDM.pdf)
- [islandmagic/bb-link (GitHub)](https://github.com/islandmagic/bb-link) — production BLE↔BT-Classic bridge for TH-D74/D75
- [LA3QMA/TH-D74-Kenwood](https://github.com/LA3QMA/TH-D74-Kenwood) — community PC command reference (51 commands)

# AX.25 + APRS — Frame Format Reference

**Date:** 2026-05-01
**Sources:**

- TAPR / Bob Bruninga, _APRS Protocol Reference_ (`APRS101.PDF`, 1998–2000)
- AX.25 Link Access Protocol for Amateur Packet Radio v2.2 (TAPR)
- Cross-checked against multiple modern decoder implementations (Direwolf, KISSer)

---

## What we'll be receiving

When the TH-D75 is in KISS mode, every Type-0 KISS Data frame contains a complete **AX.25 UI (Unnumbered Information) frame** sans the AX.25 flag bytes (0x7E) and FCS (those are stripped by the radio's TNC before the KISS layer).

Decoded, what we get is:

```
┌──────────────────────┬────────────────┬──────────────┬─────────┬─────┬────────────┐
│  Destination addr 7B │  Source addr 7B │  Digi addrs… │ Control │ PID │  Info…     │
└──────────────────────┴────────────────┴──────────────┴─────────┴─────┴────────────┘
                          (each 7B; 0..8 of them)        0x03      0xF0
```

Total frame size: 16 + 7N + InfoLen bytes, where N is digipeater count (typically 0–4) and InfoLen is 30–256.

---

## Address field (7 bytes each)

Each address is a 6-character callsign + 1 SSID/flags byte.

```
Byte 0:  ASCII char 1 << 1     (call letter, padded with space (0x20<<1=0x40) if short)
Byte 1:  ASCII char 2 << 1
Byte 2:  ASCII char 3 << 1
Byte 3:  ASCII char 4 << 1
Byte 4:  ASCII char 5 << 1
Byte 5:  ASCII char 6 << 1
Byte 6:  flags + SSID:
            bit 7  : (C bit / R bit depending on field — explained below)
            bit 6  : (C bit / R bit)
            bit 5  : R (reserved, set 1)
            bit 4  : R (reserved, set 1)
            bits 3-0: SSID (0..15)
            bit 0  : address-extension flag (0 = more follow; 1 = last address)
```

**Critical:** the address-extension bit is **bit 0 of byte 6**, NOT bit 7. The "last address" is the one whose byte-6 bit-0 is set to 1. For the digipeater chain, that means iterating address fields until a byte-6 with bit-0 set is found.

### Decoding a callsign

```cpp
char ch = (byte >> 1) & 0x7F;   // strip the shift to get ASCII
if (ch == ' ') break;           // trailing space = end of callsign
```

### SSID extraction

```cpp
uint8_t ssid_byte = addr[6];
uint8_t ssid      = (ssid_byte >> 1) & 0x0F;
bool    last      = (ssid_byte & 0x01) != 0;   // address-extension bit
bool    has_been_repeated = (ssid_byte & 0x80) != 0;  // H-bit, on digi entries only
```

### Digipeater "has been repeated" (H) bit

For digipeater addresses (everything after source until last-addr), bit 7 of the SSID byte is the **H bit** — set to 1 by each repeater that has retransmitted the frame. This is how station tracking works: counting H-bits tells you how many hops a packet took.

### Common digipeater aliases

| Path entry       | Meaning                                             |
| ---------------- | --------------------------------------------------- |
| `WIDE1-1`        | One generic 1-hop digipeater                        |
| `WIDE2-2`        | Two generic 2-hop digipeaters (decremented by each) |
| `RELAY` (legacy) | Single-hop, deprecated                              |
| `SGATE`, `IGATE` | Internet gateway flag                               |
| `GATE`           | Generic gateway                                     |

The `WIDEn-N` model: each digipeater that processes the frame **decrements N** while preserving the H-bit. When N reaches 0, the path is exhausted.

---

## Control + PID (always the same for APRS UI frames)

| Field   | Value  | Meaning                                                    |
| ------- | ------ | ---------------------------------------------------------- |
| Control | `0x03` | UI frame, no sequence numbers, no acknowledgement          |
| PID     | `0xF0` | "No Layer 3" — info field is application data (i.e., APRS) |

Any other control/PID values mean it's not standard APRS and should be passed-through or ignored.

---

## Info field — APRS data formats

The first byte of the info field is the **APRS Data Type Identifier (DTI)**:

| DTI byte | Meaning                               | Frequency in real traffic |
| -------- | ------------------------------------- | ------------------------- |
| `!` 0x21 | Position w/o timestamp, no APRS msg   | very common               |
| `=` 0x3D | Position w/o timestamp, with APRS msg | very common               |
| `/` 0x2F | Position WITH timestamp, no msg       | common                    |
| `@` 0x40 | Position WITH timestamp, with msg     | common                    |
| `;` 0x3B | Object report                         | common                    |
| `>` 0x3E | Status report                         | common                    |
| `:` 0x3A | Message (point-to-point text)         | common                    |
| `_` 0x5F | Weather report (positionless)         | sometimes                 |
| `T` 0x54 | Telemetry data                        | sometimes                 |
| `}` 0x7D | 3rd-party traffic / IGATE relay       | common via digi           |

For v0.2's **AprsApp (RX, station list)** we only need to handle position reports (`!`, `=`, `/`, `@`) and possibly status (`>`).

### Position report layout (DTI = `!` or `=`)

```
DTI(1)  Lat(8)  Sym1(1)  Lon(9)  Sym2(1)  [Comment...]
```

| Field        | Format                                              | Example     |
| ------------ | --------------------------------------------------- | ----------- |
| Latitude     | `DDMM.mmN/S` (8 chars)                              | `4903.50N`  |
| Symbol table | `/` (primary) or `\` (alternate) or overlay         | `/`         |
| Longitude    | `DDDMM.mmW/E` (9 chars)                             | `07201.75W` |
| Symbol code  | 1 char selecting the icon (e.g. `>` car, `-` house) | `>`         |
| Comment      | up to ~43 bytes of free text                        | "Mobile"    |

Lat/Lon parsing: degrees = `int(first 2 or 3 digits)`, minutes = `float(next 5 chars)`, hemisphere from final letter.

```cpp
// Lat: "4903.50N"  →  49 + 03.50/60 = 49.0583°N
// Lon: "07201.75W" → -(72 + 01.75/60) = -72.0292°
```

### Compressed position format

Same DTI bytes (`!`, `=`, `/`, `@`) but lat/lon encoded as Base-91 13-byte block. Less common than human-readable but used for high-precision reports. Specs in APRS101 §9. **v0.2 scope decision:** support uncompressed only in MVP; compressed is a v0.3 add.

---

## Typical packet sizes (for buffer sizing)

| Type                          | Total bytes (incl. AX.25 header) |
| ----------------------------- | -------------------------------- |
| Position report, no comment   | ~60–70                           |
| Position report, full comment | ~90–120                          |
| Status report                 | ~50–80                           |
| Object report                 | ~80–120                          |
| Message (text)                | 60–250                           |
| Telemetry                     | ~80                              |

**Yui buffer budget:** 256 bytes per frame is plenty. Station-list data: ~64 bytes/station × 64 stations = 4 KB, trivial.

---

## Implementation skeleton

```cpp
// include/yui/proto/Ax25.hpp
namespace yui::ax25 {

struct Address {
  char    call[7];   // NUL-terminated, up to 6 chars
  uint8_t ssid;      // 0–15
  bool    last;      // address-extension bit
  bool    repeated;  // H-bit (digipeaters only)
};

struct Frame {
  Address dst;
  Address src;
  Address digis[8];
  uint8_t digi_count;
  uint8_t control;   // 0x03 for UI
  uint8_t pid;       // 0xF0 for APRS
  const uint8_t* info;
  size_t          info_len;
};

// Parse a KISS-stripped AX.25 byte buffer. Returns false on malformed input.
bool parse(const uint8_t* buf, size_t len, Frame& out);

}  // namespace yui::ax25
```

```cpp
// include/yui/proto/Aprs.hpp
namespace yui::aprs {

struct Position {
  double  lat;        // signed degrees, +N / -S
  double  lon;        // signed degrees, +E / -W
  char    symbol_table;  // '/' or '\\'
  char    symbol_code;
  char    comment[44];
};

// Parse an APRS info field. Returns false if not a position-bearing DTI.
bool parse_position(const uint8_t* info, size_t len, Position& out);

}  // namespace yui::aprs
```

### Test approach (native unit tests)

Hand-craft byte buffers from canonical examples (the WB4APR tutorial / APRS101 has worked examples). Compare parser output against expected struct fields. Round-trip parse-then-encode for a few frames to confirm encoder symmetry once we add TX (v0.3 stretch).

Sample test vectors to capture in test data:

1. `K1ABC>APRS:!4903.50N/07201.75W>Test` — minimal position
2. `K1ABC-9>APRS,WIDE1-1:=4903.50N/07201.75W>` — with digipath, msg-capable
3. `K1ABC>APRS:>Status text` — status only
4. `K1ABC>APRS,WIDE2-1*,WIDE3-3:!...` — partial digi traversal (one hop done)

---

## ⚠️ Things that bite implementers

1. **Address byte-6 bit-0 is end-of-list, NOT bit 7.** Many AX.25 references draw the bit ordering ambiguously. Verify with sample frames.
2. **All ASCII chars in addresses are << 1.** Failing to right-shift by 1 yields garbage strings shifted upward.
3. **Padding spaces in callsigns are also shifted** (0x20 → 0x40). Trim them after right-shift.
4. **Compressed position format uses the SAME DTIs.** Detect by reading the byte after DTI: if it's `/` or `\`, it's compressed; if it's a digit, it's uncompressed.
5. **Mic-E format** is yet another encoding squeezed into the destination callsign. Common on mobile stations. Out of scope for v0.2.
6. **Non-APRS UI frames exist.** If `control != 0x03` or `pid != 0xF0`, log and drop — don't try to APRS-parse it.

---

## Open questions / TBD

- TX side (sending APRS messages): not in v0.2 must-have; deferred to stretch / v0.3. The same Ax25/Aprs structs will serve both directions.
- FCS: KISS strips the AX.25 FCS on input, and the radio adds it on TX. We don't compute it.

---

## Sources

- [APRS101.PDF — APRS Protocol Reference v1.0.1](http://www.aprs.org/doc/APRS101.PDF)
- AX.25 Link Access Protocol v2.2, TAPR (1998)
- [Direwolf software TNC source](https://github.com/wb2osz/direwolf) — most thorough open-source AX.25/APRS parser if we need a reference impl to crib structure (LICENSE: GPL, **read for ideas only, do not copy code**)

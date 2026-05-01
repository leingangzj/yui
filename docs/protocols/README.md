# Protocol audit — index

Pre-v0.2 protocol research. Each doc is read before its track's code is written.

| Track                    | Protocol(s)                                                    | Doc                                            | Status      |
| ------------------------ | -------------------------------------------------------------- | ---------------------------------------------- | ----------- |
| **A: TH-D75 (radio)**    | Bluetooth Classic SPP + Kenwood PC commands + KISS TNC         | [KENWOOD_THD75.md](./KENWOOD_THD75.md)         | ✅ Complete |
| **A: TH-D75 (APRS)**     | AX.25 UI frame + APRS info-field DTI + position parsing        | [AX25_APRS.md](./AX25_APRS.md)                 | ✅ Complete |
| **B: Pineapple**         | Hak5 Mark VII REST API (auth + dashboard + recon + PineAP)     | [PINEAPPLE_API.md](./PINEAPPLE_API.md)         | ✅ Complete |
| **C: Cardputer onboard** | ESP-IDF promiscuous mode + raw 802.11 TX + libpcap file format | [ESP_WIFI_AND_PCAP.md](./ESP_WIFI_AND_PCAP.md) | ✅ Complete |

## Top-level corrections to v0.2 plan

These are the things the audit caught **before** any v0.2 code got written:

1. **TH-D75 is Bluetooth Classic SPP, not BLE.** Update HAL naming (`IRadioLink`, not `IRadio`) and use `BluetoothSerial` (Bluedroid Classic), not NimBLE. ~50 KB extra SRAM, ~150 KB extra flash — both fit comfortably.
2. **No native Cardputer deauth.** Espressif's `esp_wifi_80211_tx` officially excludes deauth/disassoc. Patched-binary workarounds exist (Bruce, ESP32-Marauder) but are version-fragile and would be a maintenance liability. Active deauth becomes a Pineapple-driven feature instead.
3. **Pineapple network association = network swap.** When `PineappleApp` opens, the Cardputer must associate to the Pineapple's WiFi (different SSID from home). Surface this as an explicit "switching networks…" step in the UI; restore previous AP on exit.
4. **PCAP linktype = `105` (DLT_IEEE802_11)** for v0.2 — simpler, no Radiotap synthesis required. Upgrade to `127` (DLT_IEEE802_11_RADIO) only if signal-strength annotation becomes load-bearing.
5. **APRS must-have parser scope = uncompressed positions only.** Compressed (Base-91), Mic-E, and weather formats are v0.3+ scope creep; the must-have AprsApp station list works fine without them.

## Persistence keys reserved by these tracks

For continuity with existing NVS schema (`brightness`, `wifi.ssid`, `wifi.pass`, `clock.tz`, `clock.ntp`):

| Key                                           | Source  | Purpose                                           |
| --------------------------------------------- | ------- | ------------------------------------------------- |
| `radio.mac`                                   | Track A | Paired TH-D75 MAC, for auto-reconnect             |
| `radio.pin`                                   | Track A | BT pairing PIN (default `"0000"`)                 |
| `pa.host` / `pa.port` / `pa.user` / `pa.pass` | Track B | Pineapple connection                              |
| `pa.ssid` / `pa.psk`                          | Track B | The Pineapple's WiFi network (separate from home) |

NVS keys are limited to 15 chars — all the above fit.

## Open questions deferred to Phase 0 hardware bring-up

(see also `HARDWARE_AUDIT.md` for hardware-side bring-up items)

| Question                                                | Where it's tracked                     |
| ------------------------------------------------------- | -------------------------------------- |
| BT-Classic + WiFi-STA concurrent throughput             | KENWOOD_THD75.md, ESP_WIFI_AND_PCAP.md |
| Does TH-D75 send unsolicited messages on the SPP link?  | KENWOOD_THD75.md                       |
| Confirm `TN 2,0` syntax (Kenwood: space mandatory)      | KENWOOD_THD75.md                       |
| Does `AI 1\r` enable auto-info push?                    | KENWOOD_THD75.md                       |
| Pineapple recon-streaming MTU sanity (multi-KB results) | PINEAPPLE_API.md                       |
| Promiscuous-mode packet loss rate vs traffic level      | ESP_WIFI_AND_PCAP.md                   |
| SD throughput sustained for 10 MB/min `.pcap` writes    | ESP_WIFI_AND_PCAP.md                   |
| Channel-hop dwell time tuning                           | ESP_WIFI_AND_PCAP.md                   |

# ESP-IDF Promiscuous Mode + PCAP File Format

**Date:** 2026-05-01
**Sources:**

- ESP-IDF Wi-Fi API docs (`docs.espressif.com/projects/esp-idf/en/latest/esp32s3/...`)
- Espressif `esp_wifi_80211_tx` / `esp_wifi_set_promiscuous_*` API
- `libpcap` file-format spec (Wireshark wiki)
- `Jeija/esp32free80211` (deauth workaround context, now obsolete)

---

## Part 1 — ESP-IDF promiscuous mode (RX)

### Init sequence

```c
wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
esp_wifi_init(&cfg);
esp_wifi_set_storage(WIFI_STORAGE_RAM);
esp_wifi_set_mode(WIFI_MODE_NULL);          // promiscuous-only mode
esp_wifi_start();

esp_wifi_set_promiscuous_rx_cb(yui_promisc_cb);

wifi_promiscuous_filter_t filter = {
  .filter_mask = WIFI_PROMIS_FILTER_MASK_ALL  // or selectively mgmt/data
};
esp_wifi_set_promiscuous_filter(&filter);

esp_wifi_set_promiscuous(true);
esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
```

### Callback signature

```c
void yui_promisc_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
  // type: WIFI_PKT_MGMT, WIFI_PKT_CTRL, WIFI_PKT_DATA, WIFI_PKT_MISC
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf;
  // p->rx_ctrl: rssi, channel, sig_len, ...
  // p->payload[0..rx_ctrl.sig_len]: raw 802.11 frame
}
```

### Filter mask values

```c
WIFI_PROMIS_FILTER_MASK_ALL    // every type
WIFI_PROMIS_FILTER_MASK_MGMT   // beacon, probe req/resp, deauth, assoc
WIFI_PROMIS_FILTER_MASK_CTRL   // ack, rts, cts, ...
WIFI_PROMIS_FILTER_MASK_DATA   // data + qos-data
WIFI_PROMIS_FILTER_MASK_MISC   // catchall
```

For Yui:

- **WifiHandshakeApp** wants `MGMT | DATA` (mgmt for beacons + auth/assoc, data for the EAPOL handshake frames).
- **WifiProbeApp** wants `MGMT` only — probe requests + responses.

### Channel hopping

Promiscuous mode listens to **one channel at a time**. To capture across the band:

```c
for (uint8_t ch = 1; ch <= 13; ++ch) {
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  vTaskDelay(pdMS_TO_TICKS(150));   // ~150 ms dwell
}
```

5 GHz: channels 36, 40, 44, 48, 149, 153, 157, 161, 165 — but **the ESP32-S3 is 2.4 GHz only**. 5 GHz capture is Pineapple's job.

### Known limitations (per Espressif docs + community)

1. **Packet loss is real.** ESP32 promiscuous mode drops some packets, especially at high traffic. Don't expect Wireshark-grade 100% capture.
2. **Frame check sequence (FCS).** The packet payload includes the 4-byte FCS at the end. Some tools want it, some don't — for `.pcap` w/ DLT_IEEE802_11, include it.
3. **Some control packets are not delivered** even with `WIFI_PROMIS_FILTER_MASK_CTRL` — especially block-ack and ack frames responding to our own TX.
4. **No radiotap header** by default. The `wifi_pkt_rx_ctrl_t` struct has the metadata (RSSI, channel, rate) — to write to a Radiotap-format `.pcap` we'd need to synthesize the header ourselves. Easier: write `DLT_IEEE802_11` (105) and let analysis tools derive what they can.

---

## Part 2 — ESP-IDF raw 802.11 transmit (TX)

### What `esp_wifi_80211_tx()` officially supports

Per Espressif: **beacon, probe request, probe response, action, and non-QoS data frames.** Length 24–1500 bytes.

```c
esp_wifi_80211_tx(WIFI_IF_STA, frame_buf, frame_len, true /*en_sys_seq*/);
```

This is sufficient for:

- Beacon flooding (fake APs)
- Probe spoofing
- Custom data injection (e.g., Pineapple-style probe responses)

### What it explicitly does NOT support: deauth/disassoc

Espressif's API guards against transmitting management frames of type **deauth (12)** and **disassoc (10)**. The check is in the closed-source `libnet80211.a`.

### Three honest paths around it

| Path                                                                                                                         | Status                                                                                                                               | Recommendation                                                                                                                    |
| ---------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------- |
| **A. Patched `libnet80211.a`** (override `ieee80211_freedom_output` via `-zmuldefs`, technique used by Bruce/ESP32-Marauder) | Works on specific ESP-IDF versions; fragile across upgrades; the original `Jeija/esp32free80211` README declares itself **obsolete** | ❌ **Don't ship this.** Maintenance burden is real and will silently break on Espressif updates.                                  |
| **B. Use the Pineapple's `/api/pineap/deauth/*` endpoint**                                                                   | Documented, supported, works                                                                                                         | ✅ **Yes.** You own a Pineapple. The whole tri-radio plan is "Cardputer drives the Pineapple." Cardputer asks; Pineapple deauths. |
| **C. Skip deauth entirely; capture handshakes passively**                                                                    | Slower (waits for natural reassoc) but always legal and always works                                                                 | ✅ Fine for v0.2 native — just be honest about the time-to-capture.                                                               |

### v0.2 decision

- **WifiHandshakeApp on Cardputer** = passive capture only (path C). No native deauth.
- **Active deauth** = stretch goal that calls the Pineapple (path B). No patched-binary work.

This keeps Yui's Cardputer build vanilla ESP-IDF, no closed-source binary patches, no maintenance liability.

---

## Part 3 — PCAP file format (for handshake captures)

When `WifiHandshakeApp` writes captures to SD card, output `.pcap` files that Wireshark / aircrack-ng / hashcat can read directly.

### Global file header (24 bytes, written once at file start)

```
Offset  Field           Bytes  Value
0       magic           4      0xA1B2C3D4   (big-endian) or 0xD4C3B2A1 (little)
4       version_major   2      0x0002
6       version_minor   2      0x0004
8       thiszone        4      0x00000000  (GMT)
12      sigfigs         4      0x00000000
16      snaplen         4      0x0000FFFF  (65535 — max captured per packet)
20      network         4      0x00000069  (105 = DLT_IEEE802_11)
```

ESP32-S3 is little-endian, so we write `0xD4 0xC3 0xB2 0xA1` (= magic in little-endian byte order).

### Per-packet record header (16 bytes, written before each frame)

```
Offset  Field      Bytes  Value
0       ts_sec     4      Unix epoch seconds
4       ts_usec    4      microseconds within that second
8       incl_len   4      bytes actually written (≤ snaplen)
12      orig_len   4      original frame length on the wire
```

Then the raw 802.11 frame bytes (length = `incl_len`).

### LinkType (network field) options

| Value | Name                   | Notes                                                     |
| ----- | ---------------------- | --------------------------------------------------------- |
| `105` | `DLT_IEEE802_11`       | Raw 802.11 frame, no metadata header                      |
| `127` | `DLT_IEEE802_11_RADIO` | Frame prefixed with Radiotap header (RSSI, channel, rate) |
| `163` | `DLT_IEEE802_11_AVS`   | AVS header (deprecated, don't use)                        |

**For Yui v0.2:** use `105` (DLT_IEEE802_11). Simpler, no Radiotap synthesis. Good enough for handshake analysis. If we later want signal-strength annotations, switch to `127` + write a small Radiotap header per packet.

### File-write strategy on SD

- **Stream, don't buffer.** Open file, write header, then write each captured packet record as the callback fires.
- Per-record overhead = 16 bytes. ~10 MB/min is plausible for a busy capture.
- `fsync` every ~5 seconds so a power-cycle doesn't lose more than a few seconds of data.
- Filename: `/handshakes/<bssid>-<unix_ts>.pcap` so Wireshark sorts naturally and we can prune old captures by age.

### Implementation skeleton

```cpp
// include/yui/proto/Pcap.hpp
namespace yui::pcap {

struct File {
  bool open(const char* path);          // writes 24-byte header
  bool write_packet(const uint8_t* buf, size_t len, uint64_t ts_us);
  void close();
};

}  // namespace yui::pcap
```

```cpp
// Test approach: open a Pcap::File against an in-memory FakeFs, write
// 5 known packets, then byte-compare the buffer to a hand-built golden
// expected-output. No real SD, no Wireshark dependency.
TEST(Pcap, header_is_correct);
TEST(Pcap, single_packet_round_trips);
TEST(Pcap, ts_microseconds_split_correctly);
TEST(Pcap, 8KB_capture_size_matches_24+N*(16+L));
```

---

## Action items summarized

| Item                                                                          | Where it lands                                                                 |
| ----------------------------------------------------------------------------- | ------------------------------------------------------------------------------ |
| Set `WIFI_MODE_NULL` + promiscuous + filter for our two RX apps               | New helper in `Esp32Net::start_monitor(filter_mask, channel)`                  |
| Channel hopping (1–13, 150 ms dwell)                                          | Background tick in WifiHandshakeApp / WifiProbeApp                             |
| Add `IPcap` HAL (small, just `open` / `write_packet` / `close`)               | `include/yui/hal/IPcap.hpp` + `Esp32Pcap` (uses `IFs` underneath) + `FakePcap` |
| Skip Cardputer-native deauth; route active attacks through Pineapple          | v0.2 stretch (`WifiDeauthApp` calls `PineappleClient::deauth_*`)               |
| Write `DLT_IEEE802_11` (linktype 105) for v0.2; consider Radiotap (127) later | `IPcap` constructor takes a linktype enum                                      |

---

## Open questions for hardware bring-up

1. Confirm `WIFI_MODE_NULL` doesn't disable BLE on ESP32-S3 (need both BLE active for TH-D75 + monitor mode for handshake capture).
2. Measure actual packet-loss rate at typical traffic levels (pre-flight a busy AP for 5 minutes, count beacons received vs expected).
3. Verify SD throughput sustains ~10 MB/min sustained writes without dropping records.
4. Confirm channel-hop speed; some chipsets need a longer dwell on certain channels.

---

## Sources

- [ESP-IDF Wi-Fi API (latest, ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_wifi.html)
- [Wireshark libpcap file format](https://wiki.wireshark.org/Development/LibpcapFileFormat)
- [Jeija/esp32free80211 (obsolete deauth workaround, for context only)](https://github.com/Jeija/esp32free80211)

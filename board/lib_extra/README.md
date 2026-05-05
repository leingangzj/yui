# board/lib_extra/ — patched ESP-IDF archives

Drop pre-built static archives here. PlatformIO picks them up via
`lib_extra_dirs = board/lib_extra` in `platformio.ini`. Combined with
the `-Wl,-zmuldefs` linker flag, any symbol you redefine here wins over
the upstream IDF version.

## Phase 5.1 target — `libnet80211.a` (patched)

**Goal:** restore raw deauth/disassoc TX on the ESP32-S3 by overriding
Espressif's deliberately-blocked `ieee80211_freedom_output`. This is
the same workaround Bruce / ESP32-Marauder use.

**Why it's not in the repo:** the patched archive is platform- and
IDF-version-specific, and shipping a third-party blob without
provenance verification is a supply-chain risk. The operator builds
or sources it themselves, then drops it here.

### How to obtain

Either:

1. **Build from IDF source** with the deauth-block patch reverted —
   see Bruce's [build notes][bruce-build] for the exact diff against
   `components/wpa_supplicant/esp_supplicant/src/esp_wpa_main.c` and
   `components/esp_wifi/esp_wifi.c`. Resulting archive is at
   `build/esp-idf/wpa_supplicant/libwpa_supplicant.a` and
   `build/esp-idf/esp_wifi/libnet80211.a` — copy `libnet80211.a` here.

2. **Vendor a known-good binary** from a trusted upstream (e.g. the
   ESP32-Marauder release for your IDF version). Verify the SHA-256
   against the upstream release notes before committing.

[bruce-build]: https://github.com/pr3y/Bruce/blob/main/docs/build.md

### After dropping the archive in

Always run the verifier before building:

```sh
tools/verify-lib-extra.sh
```

It reads expected SHA-256 hashes from `board/lib_extra/EXPECTED_SHA256`
(one `<hash>  <filename>` per line) and refuses to pass if:

- the dropped-in archive's hash doesn't match
- an archive is present but no expected hash is recorded

When you vendor a new known-good binary, append its hash to
`EXPECTED_SHA256` in the same commit so the next operator (or your
future self) can audit the supply chain.

### Without the archive

The `-Wl,-zmuldefs` flag is harmless on its own. `DeauthApp(Native)`
will still compile and run; calls to `IWifiMonitor::tx_raw` will
return `false` (the call hits Espressif's blocked path), surfaced in
the UI as `TX failed (libnet?)`. The Pineapple backend remains fully
functional.

`HydraStatusApp` does not currently probe whether the override is in
play. If you'd like that, add a one-line check on a sentinel symbol
(future Phase 5.1.x).

### Legal reminder

Dropping the archive in here unlocks frame-injection attacks the
device cannot otherwise perform. Use only on networks you own or are
authorized to test. Combine with Faraday Mode (`SettingsApp` →
"Faraday Mode") so the strip is conditional on a controlled RF
environment.

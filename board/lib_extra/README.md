# board/lib_extra/ — patched ESP-IDF override

## What's here today

`freedom_override.c` — a single 7-byte function (`return 1`) that
overrides Espressif's `ieee80211_freedom_inside_cb`. The stock IDF
uses that callback to gate raw deauth/disassoc TX through
`esp_wifi_80211_tx`; stock returns 0 (refuse), our override returns
1 (allow), restoring native deauth.

The .c is duplicated into `src/lib_extra_override/freedom_override.c`
so PlatformIO's `build_src_filter` compiles it into the firmware's
own archive (which appears earlier in the link line than `-lnet80211`).
With `-Wl,-zmuldefs` accepting both definitions, ours wins.

**Verify it took:** after `pio run -e cardputer_adv`, run

```sh
xtensa-esp32s3-elf-gcc-nm -S .pio/build/cardputer_adv/firmware.elf \
  | grep ieee80211_freedom_inside_cb
```

Expected: size `0x07` (our override). If you see `0x2e`, Espressif's
gated version is still being linked — something has changed in the
Arduino-ESP32 link order.

## Why we ship the source, not the .a

Earlier this directory documented two paths (build patched IDF from
source, or vendor a known-good binary). Both are still valid for
operators who want a full patched `libnet80211.a`. We chose the
source-stub path because:

1. **Provenance is auditable.** The .c source is 12 lines; anyone can
   read it and convince themselves it does exactly what it claims.
2. **No supply-chain risk.** No third-party blob to verify, no IDF
   version drift.
3. **Smaller.** ~7 bytes of object code vs. a ~60 KB libnet80211.a.

If you DO want to vendor a pre-built archive (e.g. one you built
yourself from a patched IDF), drop it here and:

- append its SHA-256 to `EXPECTED_SHA256`
- update `platformio.ini` to add `lib_extra_dirs = board/lib_extra`
  AND `build_flags += -Wl,--whole-archive,board/lib_extra/<file>.a,-Wl,--no-whole-archive`
  (the `lib_extra_dirs` mechanism alone does NOT pull static archives
  into the link — confirmed by inspecting the SCons trace)
- run `tools/verify-lib-extra.sh` before any build

## Without the override (legacy build)

Comment out `+<lib_extra_override>` in `platformio.ini`'s
`build_src_filter`. Espressif's gated impl wins; `DeauthApp(Native)`
calls fall through to the stub path and the UI shows `TX failed
(libnet?)`. The Pineapple backend remains fully functional.

## Legal reminder

The override unlocks frame-injection attacks the device cannot
otherwise perform. Use only on networks you own or are authorized
to test. Combine with Faraday Mode (`SettingsApp` → "Faraday Mode")
so the strip is conditional on a controlled RF environment. App-side
gating still applies (Tab to arm + Fn+Enter to fire).

/*
 * freedom_override.c — Yui patched-libnet80211 override.
 *
 * This single function overrides Espressif's
 * `ieee80211_freedom_inside_cb`, which the stock IDF uses to gate
 * raw deauth/disassoc TX through `esp_wifi_80211_tx`. Stock impl
 * returns 0 (refuse); this returns 1 (allow), which restores
 * Bruce/ESP32-Marauder-style native deauth.
 *
 * Build:
 *   xtensa-esp32s3-elf-gcc -mlongcalls -O2 -c freedom_override.c
 *   xtensa-esp32s3-elf-ar rcs libfreedom_override.a freedom_override.o
 *
 * Linker pickup: -Wl,-zmuldefs lets our redefinition win over
 * Espressif's bundled libnet80211.a.
 *
 * LEGAL: enabling raw deauth TX without authorization violates FCC
 * Part 15 in the US and equivalent laws elsewhere. Combine with the
 * Yui FaradayMode flag so the strip is conditional on a controlled
 * RF environment. App-side gating still applies (Tab-arm + Fn+Enter).
 */
int ieee80211_freedom_inside_cb(void* ctx) {
    (void)ctx;
    return 1;
}

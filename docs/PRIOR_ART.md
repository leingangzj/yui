# Prior Art

Yui isn't the first Cardputer firmware and shouldn't pretend to be. This file catalogs what's out there, what we'd borrow ideas from, and where we diverge.

## Bruce — security toolkit

- **Repo:** <https://github.com/BruceDevices/firmware>
- **Site:** <https://bruce.computer>
- **License:** AGPL-3.0
- **Audience:** red team / pen-test hobbyists
- **Strengths:** broad feature surface (WiFi, evil portal, deauth, IR, sub-GHz, NFC, BLE), runs across the M5Stack family + LilyGo, active community.
- **What we'd borrow (ideas, not code):** modular tool registry, the way one binary targets multiple devices, menu navigation patterns, the deauth-block override trick.
- **Where we diverge:** Yui is HAL-first with a host test harness; Bruce calls Arduino APIs directly throughout. Same target, different design center.
- **License note:** AGPL is incompatible with our MIT goal. **We do not copy code from Bruce.** Reading for architecture inspiration is fine; pasting is not.

## ESP32-Marauder — WiFi/BLE attack toolkit

- **Repo:** <https://github.com/justcallmekoko/ESP32Marauder>
- **License:** MIT
- **Audience:** WiFi/BLE security researchers
- **Strengths:** mature deauth + beacon-flood + EAPOL capture path on stock ESP32 hardware, pioneered the patched `libnet80211.a` approach for raw 802.11 TX.
- **What we'd borrow (ideas):** the deauth override technique (Yui ships a 12-line C source equivalent of the override, see `board/lib_extra/`).
- **Where we diverge:** Marauder is single-platform stock-ESP32; Yui targets the Cardputer ADV with M5GFX UI and Hydra cap.

## NEMO — M5Stick + Cardputer pranks/utilities

- **Repo:** <https://github.com/n0xa/m5stick-nemo>
- **License:** GPL-2.0
- **Audience:** maker hobbyists with mostly-StickC-shaped hardware
- **Strengths:** small clean Arduino codebase, easy to read, recently added Cardputer/ADV support including BadUSB Hunter (USB-C OTG HID detection — uniquely ADV-flavored).
- **What we'd borrow (ideas):** compact app structure, single-`.ino` approach for individual tools, BadUSB Hunter as an ADV-specific trick (matches our planned `BadUsbApp` in v1.3).
- **Where we diverge:** NEMO is a single-binary Arduino sketch; Yui is a layered C++ codebase with a HAL.
- **License note:** GPL-2 is also infectious. **No code copying.**

## M5Launcher / Cardputer Launcher (official)

M5Stack's stock launcher and the burner-tool flashable images. Shows the official UX direction. Worth running on a real device once and taking notes on what feels good and what doesn't.

## Meshtastic (pre-flashed on the Mesh Kit)

Off-grid LoRa mesh comms, <https://meshtastic.org>. Pre-installed on the Cardputer ADV Mesh Kit. Yui doesn't compete on Meshtastic's own turf — if you want mesh, flash Meshtastic. A "Meshtastic client" app that talks to a separate node over BLE is a fair long-term direction.

## What's missing in the landscape

- **A hobbyist-first launcher with real test coverage.** Bruce assumes you're scanning APs; NEMO assumes you're pranking somebody. Both also call hardware APIs directly throughout, which makes them effectively untestable without flashing. Yui's HAL split means 387 host tests run in 4 seconds.
- **MIT licensing.** Bruce (AGPL) and NEMO (GPL) discourage closed forks and downstream tinkering. MIT is friendlier to "I want to grab one app and drop it into my own project."
- **Ham + pen-test in one binary.** Bruce is pen-test only; the Kenwood TH-D75 + APRS + SatTracker + Pineapple combination is the niche Yui actually fills.

These three gaps are the reason for Yui to exist. Otherwise we should just contribute to Bruce or NEMO.

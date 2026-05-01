# Prior Art

Yui is _not_ the first Cardputer firmware, nor should it pretend to be. This
file catalogs what's out there, what we'd borrow ideas from, and where we
deliberately diverge.

## Bruce — security toolkit

- **Repo:** https://github.com/BruceDevices/firmware
- **Site:** https://bruce.computer/
- **License:** AGPL-3.0
- **Audience:** red-team / pentest hobbyists
- **Strengths:** broad feature surface (WiFi tools, evil portal, deauth, IR,
  sub-GHz with CC1101, NFC, BLE), works across M5Stack family + LilyGo,
  active community.
- **What we'd borrow (ideas, not code):** modular tool registry, the way they
  let one binary target multiple devices, their menu navigation patterns.
- **Where we diverge:** Yui isn't security-focused. We won't ship deauth or
  evil portal. Hobby utility, not red-team toolkit.
- **License note:** AGPL is incompatible with our MIT goal. **We do not copy
  code from Bruce.** Reading for architecture inspiration is fine; pasting is
  not.

## NEMO — M5Stick + Cardputer pranks/utilities

- **Repo:** https://github.com/n0xa/m5stick-nemo
- **License:** GPL-2.0
- **Audience:** maker hobbyists with mostly-StickC-shaped hardware
- **Strengths:** small clean Arduino codebase, easy to read, recently added
  Cardputer/ADV support including BadUSB Hunter (USB-C OTG HID detection — a
  uniquely ADV-flavored feature).
- **What we'd borrow (ideas):** compact app structure, single-`.ino`
  approach for individual tools, BadUSB Hunter is worth looking at as an
  ADV-specific trick.
- **Where we diverge:** NEMO is a single-binary Arduino sketch; Yui is a
  layered C++ codebase with a HAL. Different design center.
- **License note:** GPL-2 also infectious. **No code copying.**

## M5Launcher / Cardputer Launcher (official)

- M5Stack's stock launcher and burner-tool flashable images.
- Shows the official UX direction. Worth running on a real device once and
  taking notes on what feels good and what doesn't.

## Meshtastic (pre-flashed on the Mesh Kit)

- Off-grid LoRa mesh comms, [meshtastic.org](https://meshtastic.org).
- Pre-installed on the Cardputer ADV Mesh Kit per the
  [2026-04-30 announcement](https://www.cnx-software.com/2026/04/30/m5stack-cardputer-goes-off-grid-with-new-mesh-kit-featuring-lora-gnss-and-meshtastic-support/).
- Yui doesn't try to compete with Meshtastic on its own turf. If you want
  mesh, flash Meshtastic. Long-term Yui could offer a "Meshtastic client" app
  that talks to a separate Meshtastic node over BLE.

## What's _missing_ in the existing landscape

- **A truly hobbyist-first launcher.** Bruce assumes you're scanning APs;
  NEMO assumes you're pranking somebody. Neither leans into "small fun
  apps for the keyboard you're holding."
- **Real test coverage.** Both Bruce and NEMO call hardware APIs directly
  throughout, which makes them effectively untestable without flashing.
- **MIT licensing.** Both Bruce (AGPL) and NEMO (GPL) discourage closed
  forks and downstream tinkering with licensing. MIT is friendlier to
  "I want to grab one app and drop it into my own project."

These three gaps are the real reason for Yui to exist. Otherwise, we should
just contribute to Bruce or NEMO.

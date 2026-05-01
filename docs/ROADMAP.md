# Roadmap

> Realistic. Personal hobby project, one developer + AI assistance, evenings
> and weekends. No funding, no team, no marketing.

## v0.0 — Foundation (this weekend)

- [x] Decide name, license, target hardware, scope
- [x] Document hardware baseline (`docs/HARDWARE.md`)
- [x] Document architecture, test plan, prior art
- [ ] Repo scaffolded, builds clean for both `cardputer_adv` and `native` envs
- [ ] HAL interfaces defined (`IDisplay`, `IKeyboard`, `IClock`, `ILog` minimum)
- [ ] Native backend: stub impls + framebuffer assertions
- [ ] One passing native unit test (e.g., menu cursor wrap)
- [ ] `cardputer_adv` env compiles a "Hello, Yui" splash that prints the build
      string to LCD
- [ ] Pushed to GitLab CT 135 with a real `git log`

**Definition of done:** `pio test -e native` is green, `pio run -e cardputer_adv`
produces a `.bin`. We do not need to flash hardware this weekend.

## v0.1 — Launcher + onboard apps (next 2–4 weekends)

Apps that need only what's onboard the bare ADV:

- [ ] **Launcher / Shell** — splash, app grid, settings, about
- [ ] **WiFi tools** — scan list, connect, info (RSSI, IP, gateway)
- [ ] **BLE scanner** — list nearby devices with name/RSSI/MAC
- [ ] **IR remote** — TV-B-Gone style code blast + custom save/replay
- [ ] **IMU toys** — bubble level, compass, step counter
- [ ] **Notes** — text editor that respects the 56-key kbd, saves to SD
- [ ] **Files** — microSD browser (list, preview, delete)
- [ ] **Calculator** — RPN-friendly, because keyboard
- [ ] **Mic visualizer** — VU meter + crude FFT bars

**Out of scope for v0.1:** anything requiring a Cap or external module. CC1101,
PN532, RFID2, LoRa, GPS, environmental sensors all wait until you actually
have one in hand.

## v0.2 — First Cap / module (when the LoRa-1262 lands)

- [ ] HAL interfaces extended (`IRadio` gets LoRa methods)
- [ ] Cap detection on EXT-14 bus
- [ ] **Meshtastic-lite app** or simple LoRa packet sender / receiver

(Other modules — PN532, BME680, etc. — slot in as they arrive. Each becomes a
small driver + one new app.)

## v1.0 — When it's "done enough"

A Yui v1.0 release is "I'd be happy to hand this to a friend with an ADV and
have them not be confused." That means:

- Stable launcher, no crashes during a day's use
- 5+ apps that actually do something useful
- README + install instructions a non-coder can follow (M5Burner image)
- At least one app that's more interesting than "Bruce already does this"

No date. We get there when we get there.

## Explicit non-goals

- **Multi-device support** in v1. Cardputer ADV only. Don't add complexity
  for the base Cardputer or T-Deck until v0.1 actually ships.
- **OTA updates** in v1. Wired flashing is fine for a hobby project.
- **A custom UI library.** M5GFX is fine; we're not a graphics project.
- **Web companion app.** Maybe later, definitely not first.
- **Plugin marketplace.** Apps are PRs, not downloadable bundles. Keep it
  simple.

## How we'll measure progress

Every weekend: did we ship one observable thing? An app that runs, a test that
passes, a doc that didn't exist before. If three weekends in a row pass with
no observable shipped thing, the project is dead and we should admit it
instead of generating more documentation.

# Yui v1.0 — Demo Video Shot List

The "done enough for v1.0" definition from `docs/ROADMAP.md`:

> A friend with an ADV reads the README, flashes the M5Burner image, pairs
> it with their TH-D75 over Bluetooth, points it at an upcoming ISS pass,
> and works the bird. No phone, no laptop. They also use it as a hand-held
> remote for their Pineapple at a paid pentest gig. Both work first try.

This shot list translates that into a ~3-minute demo. Three acts mirror the
three v1.0 hero scenarios: **TH-D75 + SatTracker** (ham radio), **Pineapple**
(pentest), **standalone tools** (everyday carry).

---

## Pre-shoot prep

- [ ] **Hardware staged:** ADV with v1.0 firmware, charged ≥80%; TH-D75 with bb-link bridge paired; WiFi Pineapple Mark VII (or Nano) booted and on its own SSID; phone with a stopwatch app (for ISS pass timing).
- [ ] **Software staged:** ADV's `/yui-tles.txt` populated with current ISS + SO-50 + AO-91 TLEs from CelesTrak (refreshed within 7 days); Pineapple credentials in NVS via SettingsApp; WiFi credentials in NVS for the home AP that gets you to the Pineapple.
- [ ] **Pass picked:** use Heavens-Above or `predict` — find an ISS pass with max-elevation ≥30° in the next 90 min. Log AOS / max / LOS times. **Re-check 5 min before shooting** — TLEs drift.
- [ ] **Antenna:** dual-band (2 m / 70 cm) yagi or arrow antenna for the satellite work; rubber duck is fine for the Pineapple act.
- [ ] **Camera setup:** primary on a tripod for the macro screen shots; phone or second cam for hand-held wide shots showing the whole device + radio.
- [ ] **Audio:** ambient mic for room sound; pull TH-D75 audio direct via the headphone jack into the camera if you have a 3.5 mm splitter, otherwise just live-room.

**Light:** soft side light. The ST7789V2 looks washed out under direct overhead.

---

## Cold open (0:00–0:10)

- [ ] **0.1** Hand-held shot, ADV closed in palm, slow rotate. One line lower-third: **"Yui — pocket comms + pentest companion."**
- [ ] **0.2** Cut to ADV waking up: hinomaru-koi splash → launcher categories. Hold on the Tools/Fun/WiFi/Ble/Radio/System hero shot for 2 s.

**Why this opens the demo:** the splash is the only piece of original art in the project, and the categories make it obvious the device isn't a single-purpose toy. If the koi animation doesn't render, abort the shoot and fix the bug — that's the brand.

---

## Act 1: TH-D75 + SatTracker — work an ISS pass (0:10–1:30)

This is the marquee scene. The whole point of bb-link + SatTracker is that you can work a satellite from a coat pocket with no laptop.

- [ ] **1.1** Wide: tester wearing the antenna, ADV in left hand, TH-D75 in right. Voiceover: **"ISS comes over in three minutes. We don't have a laptop. We have this."**
- [ ] **1.2** Macro on the ADV screen: open **Radio → Kenwood**. Show the bb-link "B.B. Link" advertisement, tap connect, status flips to `CommandMode`. Audible: TH-D75 chirps when it gets the bridge handshake (if audio split is in place).
- [ ] **1.3** Macro: open **Radio → SatTracker**. List shows ISS at the top with a countdown to AOS. Cursor down to ISS, Enter → Detail view: shows az/el/range plus next pass info.
- [ ] **1.4** **30 s before AOS:** tester aims antenna at calculated AOS azimuth (callout overlay: "AOS 287° / max 71° / LOS 102°"). Tab → Live mode.
- [ ] **1.5** Macro: SatTracker Live screen — frequency display shows the TH-D75 receive freq updating in real time. Doppler delta text shows ±2-3 kHz at AOS, drifting through 0 at TCA, then negative.
- [ ] **1.6** Wide: tester transmits a short voice ID — "**KX0XXX, grid EM48**" (insert real call). Cut to TH-D75 screen showing PTT-out and any ack-bar. Listen: ISS repeater audio comes back through the radio.
- [ ] **1.7** Live cut between ADV freq updating + radio audio. Hold on the Live screen as the count-up timer crosses TCA — show that the Doppler delta crosses zero exactly when expected.
- [ ] **1.8** **At LOS:** tester turns off PTT, taps Esc on the ADV (Live → Detail → List). Status returns to `CommandMode`. Voiceover: **"That was the ISS, worked from a coat pocket, hands-free Doppler."**

**Risk callouts to address in voiceover:**

- bb-link is a separate ~$5 ESP32 — name-drop the project (`islandmagic/bb-link`).
- TLEs were fresh — say so.
- The TH-D75 is BT-Classic only, which is why the bridge exists at all.

---

## Act 2: Pineapple Recon (1:30–2:30)

Same device, totally different scenario. The point is: **everyday carry, two jobs**.

- [ ] **2.1** Wide: tester at a café-table mockup, laptop visibly _closed_, ADV in hand. Voiceover: **"Now: a paid recon gig. Same device, no laptop."**
- [ ] **2.2** Macro on screen: **WiFi → Wifi** shows connected to the Pineapple's management SSID (or to a separate AP with reachable Pineapple). RSSI visible in status bar.
- [ ] **2.3** Open **WiFi → PineappleRecon**. Tab to start — the Pineapple's PineAP results stream in: SSIDs, BSSIDs, client probes. Show 8-12 entries scroll by.
- [ ] **2.4** Cursor over an interesting SSID, Enter for detail. Screen shows associated stations + handshake-capture state.
- [ ] **2.5** Switch to **WiFi → CaptivePortal** (run _only_ against your own gear). Fn+Enter → SoftAP starts. Connect a second device (phone) to the captive SSID; phone's auto-portal pops up; submit dummy creds. Macro on ADV: capture line appears, Tab writes to SD.
- [ ] **2.6** Open **Tools → Files**, browse to the captive log. Open the file, show the captured POST line. Backspace out, then ESC out of the SoftAP.
- [ ] **2.7** Voiceover wrap: **"Native pentest tools. SD-logged. No tethering."**

**Operational reminder for the talent (off-camera):** every WiFi-deauth / beacon-flood / handshake-capture demo must be against gear you own. Do NOT shoot against neighbor's APs even for B-roll. Show the same gear in two scenes if you need range cuts.

---

## Act 3: Everyday carry montage (2:30–2:50)

Quick cuts to make the point that the launcher is a real shell, not a single-app demo.

- [ ] **3.1** **Calculator** RPN: `2 Enter 3 +` → 5; `45 d sin` (deg sine).
- [ ] **3.2** **Notes** open, type a few lines, Tab to save. Cut to **Files** → see the file appear at `/yui-notes.txt`.
- [ ] **3.3** **Mic** with FFT bands jumping to room sound (or whistle).
- [ ] **3.4** **Clock** TimeOfDay showing accurate local time.
- [ ] **3.5** **Snake** or **Life** for a half-second of "yes, also fun."
- [ ] **3.6** **IR Remote** firing at a TV in the background — TV visibly turns off.

---

## Outro (2:50–3:00)

- [ ] **4.1** Hold on the launcher's category screen, fade lower-third: **"leingangzj/yui — MIT — fits in a coat pocket."**
- [ ] **4.2** Cut to black.

---

## Capture order on the day

Don't shoot in narrative order. Shoot in this order to minimize re-staging:

1. **Pre-pass setup + charging shots** (any time).
2. **Act 3 montage** — 8–10 minutes of free-form everyday-carry shots; pick the best later.
3. **Act 2 Pineapple** — needs the Pineapple powered + range; do this during a known-quiet RF window.
4. **Cold open + outro** — needs only the device, can re-shoot anytime.
5. **Act 1 satellite** — last, and timed to the actual pass. **You only get one take.** If you miss AOS, wait for the next pass.

If the satellite act fails (no signal, antenna issue, bb-link drops), don't fake it — re-shoot at the next pass. The ham-radio audience will catch a faked Doppler in two seconds.

---

## Cut list deliverable

Edit produces three exports:

- `yui-v1.0-demo-3min.mp4` — full demo, 1080p60, H.264 high.
- `yui-v1.0-demo-30s.mp4` — Act 1 only (the satellite hero shot), for social.
- `yui-v1.0-stills/` — 6-8 still frames from the screen for the README + GitLab tile.

All three live alongside the release binary in `release/`. None of them go into the repo binary — link out from README to a release page.

---

## Failure-mode reshoots

| If this breaks                      | Reshoot strategy                                                                   |
| ----------------------------------- | ---------------------------------------------------------------------------------- |
| Splash doesn't render               | Stop. File issue. Don't shoot demo until fixed.                                    |
| bb-link won't pair                  | Power-cycle bridge; verify bb-link firmware version; reshoot.                      |
| ISS pass clouded by RF interference | Pick AO-91 (FM repeater) or SO-50 instead — Act 1 still works.                     |
| Pineapple unreachable               | Skip Act 2.6 captive portal; keep Recon scene; voiceover adapts.                   |
| TV won't respond to IR              | Drop step 3.6 (it's the easiest cut).                                              |
| Battery dies mid-shoot              | Plug in for the screen-macro shots; only need it untethered for the satellite act. |

# Yui UI conventions

The Cardputer ADV is 240×135 with rotation 1. That's not a lot of room.
Every app shares the same chrome so the user doesn't have to relearn the
geometry app-to-app.

## Font system (Phase 1)

Every text path in the firmware lands at
`IDisplay::draw_text_styled(x, y, s, fg, bg, FontStyle)`. Pick a style
and the backend picks the font:

| `FontStyle` | Esp32 font            | Native | Use for                         |
| ----------- | --------------------- | ------ | ------------------------------- |
| `Title`     | `FreeSansBold12pt7b`  | 12×16  | Page headers, splash titles     |
| `Body`      | `FreeSans9pt7b`       | 6×8    | Default reading size, list rows |
| `Caption`   | `Font0` (default 6×8) | 6×8    | Hints, footers, secondary stats |
| `Mono`      | `Font0`               | 6×8    | Hex / freq / fixed-width values |

Apps that don't pick a style — `d.draw_text(x, y, s, fg, bg)` — get
Body. Layout queries (`d.text_width(s, style)`,
`d.line_height(style)`) replace hardcoded `kCharW * strlen()` math when
you need real pixel sizing.

Legacy `draw_text_scaled(..., scale)` still works: `1 → Caption`, `2 →
Body`, `≥3 → Title`. Existing callers harmonize automatically.

## Layout grid

```
┌───────────────────────────────────────┐  y = 0
│  Header  (red, Title-style)           │  24 px tall
├───────────────────────────────────────┤  y = 24
│  ↕ 4 px breathing room                │
│  Body                                 │  y = 28..120
│  · Body-style content (~13 px)        │
│  · 8 px left/right padding            │
│                                       │
├───────────────────────────────────────┤  y = 123
│  Footer hint (dim red, Caption)       │  12 px
└───────────────────────────────────────┘  y = 135
```

All numbers above are exposed in `include/yui/ui/Tokens.hpp` —
`kHeaderH`, `kBodyTopY`, `kBodyLineH`, `kFooterY`, `kBodyPadX`,
`kListRowH`. Read tokens, don't hard-code.

## Chrome helpers

Use `include/yui/ui/Chrome.hpp` for everything that paints around your
content. Existing primitives (now using the styled-font system):

| Call                                                | When                          |
| --------------------------------------------------- | ----------------------------- |
| `Chrome::header(d, title, subtitle?)`               | Always, top of every render() |
| `Chrome::footer(d, hint)`                           | Always — even just "Esc:back" |
| `Chrome::list_row(d, row, label, sel, icon?, len?)` | Any vertical selectable list  |
| `Chrome::badge(d, x, y, text, on)`                  | Inline mode/state indicator   |
| `Chrome::empty(d, "...", &koi)`                     | Empty / pre-data screen       |
| `Chrome::stat(d, row, "Label", "value")`            | Stat-row body                 |

New in Phase 1:

| Call                                                     | When                        |
| -------------------------------------------------------- | --------------------------- |
| `Chrome::scrollbar(d, top_y, h, total, visible, top)`    | List overflow indicator     |
| `Chrome::var_item_row(d, row, label, val, sel, edit)`    | `Setting: < value >` editor |
| `Chrome::dialog(d, title, msg, primary, secondary, sel)` | Modal alert / confirmation  |

`list_row` now takes optional `icon_png` + `icon_len` — pass PNG bytes
for a 24×24 left-side icon (Phase 2 wiring). Pass `nullptr, 0` to keep
the old text-only row.

The header title prints in `FontStyle::Title`. Keep titles ≤ 19 chars.
The optional subtitle prints in `FontStyle::Caption`, right-aligned —
use it for mode/state/count, not for description.

## Mascot rules

The koi is a **mascot, not decoration.** Use it where it carries info:

- `Chrome::empty(...)` — small idle-koi sprite next to "no data yet"
  messages reads as friendly idle, not as a placeholder
- `KoiGotchiApp` — the koi _is_ the UI; mood reflects EAPOL state
- Splash — boot animation; already wired

Don't drop a koi into a settings list "to liven it up." Restraint keeps
the mascot meaningful when it shows up.

## Typography

- M5GFX default font, sizes 1 (6×8) and 2 (12×16). Don't introduce
  others — every additional font costs flash.
- Title bar: size 2.
- Body and footer: size 1.
- Use scale only for the title and rare hero numbers (Pomodoro
  countdown, Snake score). Scaled text in lists makes the screen
  feel cramped fast.

## Colors

```
kSurface     #FFFFFF   background
kOnSurface   #000000   primary body text
kAccent      #BC002D   header bar, selection, badges-on, primary
kOnAccent    #FFFFFF   text on accent
kAccentDark  #780000   hints, footers, dim-on-white
kHint        kAccentDark   alias — use wherever the role is "hint"
kWarn        #FF4060   errors, failed states, and "firing right now"
                       indicators (deauth armed, beacon flood enabled,
                       etc.) — louder than accent on purpose
```

Three rules:

1. **Accent is loud.** Use it for one thing per screen — usually the
   header. Selection bars and badges are also accent, but only one of
   those should dominate at a time.
2. **kHint, not kOnSurface, for secondary text.** kBlack on kWhite
   reads as primary content; kHint (dark red) reads as guidance.
3. **No raw hex literals in app code.** Always reference a color token
   from `types.hpp` or `Tokens.hpp`. If you find yourself wanting a
   shade we don't have, pause — likely the right answer is hint+pad.

## Selection idiom

Vertical lists use the launcher sub-view pattern:

- Selected row: full-width red bar with white text
- Other rows: white background with red text
- Cursor is always visible; never relegate it to a marker character

`Chrome::list_row` enforces this.

## Footer hints

Every screen has one. The format is `"Key:action  Key:action"` with two
spaces between actions. Keep total length ≤ 38 chars (just under the
240/6 char limit at size 1). Common idioms:

- `"Esc:back"` — leaf screens with no input
- `"Enter:open  Esc:back"` — selectable list
- `"Tab:save  Esc:back"` — editor
- `"Space:start  Esc:back"` — toggleable runs

If the screen needs more, add a second-tier hint at `kFooterY - 14` —
but reach for that sparingly; if you can't summarize it in one line you
probably need a sub-view.

## Status / right-side header subtitle

Anything that varies per render — channel number, packet count, mode —
goes in `Chrome::header`'s subtitle slot, not as a body line. Frees up
a body row and keeps the changing data at a fixed location the user
can glance at.

Examples:

- `Handshake` + subtitle `"ch 6"` → channel hop indicator
- `WiFi` + subtitle `"42"` → network count
- `Notes` + subtitle `"*"` → unsaved-dirty indicator

## What this audit didn't refactor

Only 5 apps were migrated to Chrome in the conversion commit:
`AboutApp`, `RemoteApp`, `WifiHandshakeApp`, `IrRemoteApp`,
`KoiGotchiApp`. The other ~35 still use the old hand-coded chrome —
they'll be migrated as we touch them. Don't add new apps without
Chrome — the helpers are mandatory for new code.

# UI conventions

The Cardputer ADV is 240×135 with rotation 1. Not a lot of room. Every app shares the same chrome so the user doesn't have to relearn the geometry app to app.

## Font system

Every text path in the firmware lands at `IDisplay::draw_text_styled(x, y, s, fg, bg, FontStyle)`. Pick a style and the backend picks the font:

| `FontStyle` | ESP32 font            | Native | Use for                         |
| ----------- | --------------------- | ------ | ------------------------------- |
| `Title`     | `FreeSansBold12pt7b`  | 12×16  | Page headers, splash titles     |
| `Body`      | `FreeSans9pt7b`       | 6×8    | Default reading size, list rows |
| `Caption`   | `Font0` (default 6×8) | 6×8    | Hints, footers, secondary stats |
| `Mono`      | `Font0`               | 6×8    | Hex / freq / fixed-width values |

Apps that don't pick a style — `d.draw_text(x, y, s, fg, bg)` — get Body. Layout queries (`d.text_width(s, style)`, `d.line_height(style)`) replace hardcoded `kCharW * strlen()` math when you need real pixel sizing.

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

All numbers above are exposed in `include/yui/ui/Tokens.hpp` — `kHeaderH`, `kBodyTopY`, `kBodyLineH`, `kFooterY`, `kBodyPadX`, `kListRowH`. Read tokens, don't hard-code.

## Chrome helpers

`include/yui/ui/Chrome.hpp` paints everything around your content.

| Call                                                            | When                                                |
| --------------------------------------------------------------- | --------------------------------------------------- |
| `Chrome::header(d, title, subtitle?)`                           | Top of every render()                               |
| `Chrome::radio_header(d, title, subtitle, cc_state, nrf_state)` | Radio category apps — adds Hydra cap indicator dots |
| `Chrome::footer(d, hint)`                                       | Always — even just "Esc:back"                       |
| `Chrome::list_row(d, row, label, sel, icon?, len?)`             | Any vertical selectable list                        |
| `Chrome::badge(d, x, y, text, on)`                              | Inline mode/state indicator                         |
| `Chrome::empty(d, "...", &koi)`                                 | Empty / pre-data screen                             |
| `Chrome::stat(d, row, "Label", "value")`                        | Stat-row body                                       |
| `Chrome::scrollbar(d, top_y, h, total, visible, top)`           | List overflow indicator                             |
| `Chrome::var_item_row(d, row, label, val, sel, edit)`           | `Setting: < value >` editor                         |
| `Chrome::dialog(d, title, msg, primary, secondary, sel)`        | Modal alert / confirmation                          |
| `Chrome::cap_missing_dialog(d, app, chip, cs)`                  | RF apps when the Hydra cap isn't seated             |

Header titles use `FontStyle::Title`. Keep them ≤ 19 chars. Subtitles use `FontStyle::Caption`, right-aligned — for mode/state/count, not description.

## Footer convention

`"Key:action Key:action ..."` format, single space between actions. Total length ≤ 38 chars (just under the 240/6 char limit at size 1).

Standard keybinding strings (apply the same convention everywhere):

| Token      | Means                      |
| ---------- | -------------------------- |
| `</>`      | Left/Right arrows          |
| `^/v`      | Up/Down arrows             |
| `Tab`      | secondary action / cycle   |
| `Enter`    | activate                   |
| `Fn+Enter` | armed action (RF TX, etc.) |
| `Bksp`     | backspace / stop / disable |
| `Esc`      | back                       |

Common idioms:

- `"Esc:back"` — leaf screen with no input
- `"Enter:open Esc:back"` — selectable list
- `"Tab:save Esc:back"` — editor
- `"</> change Esc:back"` — single-axis tuner

If the screen needs more than one footer line, add a second tier at `kFooterY - 14`. Reach for it sparingly; if you can't summarize in one line you probably need a sub-view.

## Color tokens

```
kSurface     #FFFFFF      background
kOnSurface   #000000      primary body text
kAccent      #BC002D      header bar, selection, badges-on, primary
kOnAccent    #FFFFFF      text on accent
kAccentDark  #780000      hints, footers, dim-on-white
kHint        kAccentDark  alias — use wherever the role is "hint"
kWarn        #FF4060      errors, failed states, "firing right now" indicators
                          (deauth armed, beacon flood enabled, etc.) —
                          louder than accent on purpose
```

Three rules:

1. **Accent is loud.** Use it for one thing per screen — usually the header. Selection bars and badges are also accent, but only one of those should dominate at a time.
2. **`kHint`, not `kOnSurface`, for secondary text.** kOnSurface (black on white) reads as primary content; kHint (dark red) reads as guidance.
3. **No raw hex literals in app code.** Always reference a color token from `Tokens.hpp`. If you want a shade we don't have, pause — likely the right answer is hint+pad.

## Selection idiom

Vertical lists use the launcher sub-view pattern:

- Selected row: full-width red bar with white text
- Other rows: white background with red text
- Cursor is always visible; never relegate it to a marker character

`Chrome::list_row` enforces this.

## Subtitle data

Anything that varies per render — channel, packet count, mode — goes in `Chrome::header`'s subtitle slot, not as a body line. Frees up a body row and keeps changing data at a fixed glance location.

Examples:

- `Handshake` + subtitle `"ch 6 PMKID"` → channel + mode
- `WiFi` + subtitle `"42 APs"` → scan count
- `Sub-GHz Jam` + subtitle `"Active"` → state

Use Title Case for state words: `Active`, `Jamming`, `Done`, `Stopped`, `Ok`, `Partial`, `Missing`. (Earlier code mixed ALL CAPS with Title Case; v1.2 harmonized.)

## Capitalization

App names: Title Case (`Sub-GHz Capture`, `Hydra Status`).
Subtitles / state: Title Case (`Active`, `Recording`, `Done`).
Footer key tokens: lowercase action (`Tab:cycle`, `Enter:fire`, `Esc:back`).

#pragma once
// Single-page browser viewer for the Yui remote server. Served as / from
// the device's HTTP listener. ~3 KB; PROGMEM-safe (const char[] in flash).
//
// Protocol: see Esp32WebRemote.hpp.

#if defined(YUI_TARGET_CARDPUTER_ADV)
#include <pgmspace.h>
#else
#define PROGMEM
#endif

namespace yui {

inline const char remote_viewer_html[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Yui Remote</title>
<style>
  :root { color-scheme: dark; }
  body { margin:0; background:#111; color:#eee; font:14px/1.4 system-ui,sans-serif;
         display:flex; flex-direction:column; align-items:center; min-height:100vh; }
  header { padding:8px 12px; display:flex; gap:12px; align-items:center;
           background:#1a1a1a; width:100%; box-sizing:border-box; }
  header h1 { font-size:14px; margin:0; color:#bc002d; letter-spacing:1px; }
  header .stat { color:#888; font-family:ui-monospace,monospace; }
  header .ok { color:#7c7; }
  header .bad { color:#c66; }
  main { flex:1; display:flex; align-items:center; justify-content:center; padding:16px; }
  canvas { background:#000; image-rendering:pixelated;
           box-shadow:0 0 0 1px #333, 0 8px 30px rgba(0,0,0,0.6); }
  footer { padding:8px 12px; color:#666; font-size:12px; }
  button { background:#222; color:#eee; border:1px solid #444;
           padding:4px 10px; border-radius:3px; cursor:pointer; font:inherit; }
  button:hover { background:#2a2a2a; }
  button[aria-pressed="true"] { background:#bc002d; border-color:#bc002d; color:#fff; }
</style></head><body>
<header>
  <h1>YUI · REMOTE</h1>
  <span class="stat" id="stat">connecting…</span>
  <span class="stat" id="fps">0 fps</span>
  <span style="flex:1"></span>
  <button id="s1" aria-pressed="false">1×</button>
  <button id="s2" aria-pressed="true">2×</button>
  <button id="s4" aria-pressed="false">4×</button>
</header>
<main><canvas id="c" width="240" height="135" tabindex="0"></canvas></main>
<footer>Click the canvas, then type. Arrow keys / Enter / Esc / Tab / Backspace map straight through.</footer>
<script>
(() => {
  const canvas = document.getElementById('c');
  const stat = document.getElementById('stat');
  const fpsEl = document.getElementById('fps');
  const ctx = canvas.getContext('2d');
  const W = canvas.width, H = canvas.height;
  const img = ctx.createImageData(W, H);

  let scale = 2;
  const setScale = s => {
    scale = s;
    canvas.style.width = (W*s)+'px';
    canvas.style.height = (H*s)+'px';
    for (const id of ['s1','s2','s4']) {
      document.getElementById(id).setAttribute('aria-pressed',
        (id === 's'+s) ? 'true' : 'false');
    }
  };
  document.getElementById('s1').onclick = () => setScale(1);
  document.getElementById('s2').onclick = () => setScale(2);
  document.getElementById('s4').onclick = () => setScale(4);
  setScale(2);

  // RGB565 LE → RGBA8888.
  const decodeFrame = (buf) => {
    const dv = new DataView(buf, 4);  // skip 4-byte header
    const px = img.data;
    let i = 0;
    for (let p = 0; p < W*H; ++p) {
      const v = dv.getUint16(p*2, true);
      const r = ((v >> 11) & 0x1f) * 255 / 31;
      const g = ((v >> 5)  & 0x3f) * 255 / 63;
      const b = ( v        & 0x1f) * 255 / 31;
      px[i++] = r; px[i++] = g; px[i++] = b; px[i++] = 255;
    }
    ctx.putImageData(img, 0, 0);
  };

  // Browser key → device Key enum (must match include/yui/hal/IKeyboard.hpp).
  const KEY = { None:0, Up:1, Down:2, Left:3, Right:4, Enter:5, Esc:6,
                Tab:7, Backspace:8, Space:9, Fn:10, Shift:11, Ctrl:12,
                Alt:13, Char:14 };
  const keymap = {
    'ArrowUp':KEY.Up, 'ArrowDown':KEY.Down,
    'ArrowLeft':KEY.Left, 'ArrowRight':KEY.Right,
    'Enter':KEY.Enter, 'Escape':KEY.Esc, 'Tab':KEY.Tab,
    'Backspace':KEY.Backspace, 'Space':KEY.Space,
    'ShiftLeft':KEY.Shift, 'ShiftRight':KEY.Shift,
    'ControlLeft':KEY.Ctrl, 'ControlRight':KEY.Ctrl,
    'AltLeft':KEY.Alt, 'AltRight':KEY.Alt,
  };

  // WebSocket lives on :81 by convention; same host, derived from page URL.
  const wsUrl = `ws://${location.hostname}:81/`;
  let ws, frames = 0, fpsTimer = 0;

  const connect = () => {
    stat.textContent = 'connecting…';
    stat.className = 'stat';
    ws = new WebSocket(wsUrl);
    ws.binaryType = 'arraybuffer';
    ws.onopen    = () => { stat.textContent = 'connected'; stat.className = 'stat ok'; };
    ws.onclose   = () => { stat.textContent = 'disconnected'; stat.className = 'stat bad';
                           setTimeout(connect, 1000); };
    ws.onerror   = () => { stat.textContent = 'error'; stat.className = 'stat bad'; };
    ws.onmessage = (e) => {
      if (typeof e.data === 'string') return;
      const view = new Uint8Array(e.data);
      if (view[0] === 0x01) { decodeFrame(e.data); ++frames; }
    };
  };
  connect();

  fpsTimer = setInterval(() => {
    fpsEl.textContent = frames + ' fps';
    frames = 0;
  }, 1000);

  const sendKey = (e, down) => {
    if (!ws || ws.readyState !== 1) return;
    const code = keymap[e.code];
    let key = code, ascii = 0;
    if (code === undefined) {
      if (e.key.length === 1) {
        key = KEY.Char;
        ascii = e.key.charCodeAt(0) & 0x7f;
      } else return;
    }
    const mods = (e.shiftKey?0x01:0) | (e.ctrlKey?0x02:0) |
                 (e.altKey?0x04:0)   | (e.metaKey?0x08:0) |
                 (down?0:0x80);
    const buf = new Uint8Array([0x10, key & 0xff, mods, ascii]);
    ws.send(buf);
    e.preventDefault();
  };
  canvas.addEventListener('keydown', e => sendKey(e, true));
  canvas.addEventListener('keyup',   e => sendKey(e, false));
  canvas.addEventListener('click',   () => canvas.focus());
  canvas.focus();
})();
</script>
</body></html>
)HTML";

}  // namespace yui

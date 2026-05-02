#!/usr/bin/env python3
"""Yui soak-test monitor.

Tails the Cardputer ADV's USB-CDC serial output for a long run (default 24 h),
captures every line to a log, and flags anomalies in real time:

  - panic / abort / Guru Meditation / WDT crashes
  - heap drop > kHeapDropPctAlert vs the running median
  - IP loss (WiFi disconnect)
  - BLE disconnects from the bb-link bridge
  - rapid boot loop (Yui banner appears more than kBootLimit times)

Run:
  tools/soak-monitor.py --port /dev/ttyACM0 --hours 24

Output:
  release/soak-<isots>.log         — raw line-by-line transcript
  release/soak-<isots>-summary.md  — markdown summary written every 5 min and at exit

Requires pyserial. The platformio bundle has it; either source the venv or use
its python:
  ~/.platformio/penv/bin/python3 tools/soak-monitor.py ...
"""

from __future__ import annotations

import argparse
import datetime as dt
import os
import re
import signal
import statistics
import sys
import time
from collections import Counter, deque
from pathlib import Path

try:
    import serial
except ImportError:
    sys.stderr.write(
        "pyserial not found. Run with platformio's python:\n"
        "  ~/.platformio/penv/bin/python3 tools/soak-monitor.py ...\n"
    )
    sys.exit(2)


# ─── Tunables ────────────────────────────────────────────────────────────────

kHeapDropPctAlert = 25         # pct drop vs running median that trips an alert
kHeapWindow       = 64         # samples kept for the running heap baseline
kBootLimit        = 3          # >3 boots in a soak = boot-loop alert
kSummaryInterval  = 300        # seconds between rolling summary writes
kReadTimeout      = 1.0        # serial read timeout (seconds)


# ─── Patterns ────────────────────────────────────────────────────────────────

PAT_PANIC       = re.compile(r"(Guru Meditation|panic|abort\(\)|backtrace:|assertion failed)", re.I)
PAT_WDT         = re.compile(r"(Task watchdog|WDT|wdt timeout)", re.I)
PAT_HEAP        = re.compile(r"heap[_ ]free[:= ]+(\d+)", re.I)
PAT_BOOT_BANNER = re.compile(r"Yui v[0-9]")
PAT_WIFI_DOWN   = re.compile(r"(WiFi.*disconnected|sta:disconnect|disassoc reason)", re.I)
PAT_BLE_DOWN    = re.compile(r"(BLE.*disconnected|onDisconnect|ble[_ ]disconnect)", re.I)


# ─── Soak monitor ────────────────────────────────────────────────────────────


class SoakMonitor:
    def __init__(self, port: str, baud: int, hours: float, outdir: Path):
        self.port = port
        self.baud = baud
        self.duration = hours * 3600.0
        self.outdir = outdir
        outdir.mkdir(parents=True, exist_ok=True)
        ts = dt.datetime.now().strftime("%Y%m%dT%H%M%S")
        self.log_path = outdir / f"soak-{ts}.log"
        self.summary_path = outdir / f"soak-{ts}-summary.md"

        self.start = time.monotonic()
        self.heap_samples: deque[int] = deque(maxlen=kHeapWindow)
        self.events: Counter[str] = Counter()
        self.boots = 0
        self.panics: list[tuple[float, str]] = []
        self.wdts: list[tuple[float, str]] = []
        self.heap_drops: list[tuple[float, int, float]] = []
        self.last_summary = self.start

    # ─── Lifecycle ───────────────────────────────────────────────────────────

    def run(self) -> int:
        print(f">>> Soak start — port={self.port} baud={self.baud} duration={self.duration/3600:.1f}h")
        print(f">>> Log: {self.log_path}")
        print(f">>> Summary: {self.summary_path}")

        try:
            ser = serial.Serial(self.port, self.baud, timeout=kReadTimeout)
        except serial.SerialException as exc:
            sys.stderr.write(f"serial open failed: {exc}\n")
            return 2

        signal.signal(signal.SIGINT, self._sigint)
        signal.signal(signal.SIGTERM, self._sigint)

        with self.log_path.open("w", buffering=1) as logf:
            while time.monotonic() - self.start < self.duration:
                try:
                    raw = ser.readline()
                except serial.SerialException as exc:
                    self._note(logf, f"[soak] serial error: {exc}")
                    time.sleep(1.0)
                    continue
                if not raw:
                    self._maybe_summary()
                    continue
                line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
                ts = time.monotonic() - self.start
                logf.write(f"{ts:9.2f}  {line}\n")
                self._inspect(ts, line)
                self._maybe_summary()

        self._write_summary(final=True)
        print(">>> Soak complete.")
        return 0

    # ─── Inspection ──────────────────────────────────────────────────────────

    def _inspect(self, ts: float, line: str) -> None:
        if PAT_BOOT_BANNER.search(line):
            self.boots += 1
            self.events["boot"] += 1
            if self.boots > kBootLimit:
                self._note_alert(f"BOOT-LOOP: {self.boots} banners seen at t+{ts:.1f}s")

        if PAT_PANIC.search(line):
            self.panics.append((ts, line))
            self.events["panic"] += 1
            self._note_alert(f"PANIC at t+{ts:.1f}s: {line[:120]}")

        if PAT_WDT.search(line):
            self.wdts.append((ts, line))
            self.events["wdt"] += 1
            self._note_alert(f"WDT at t+{ts:.1f}s: {line[:120]}")

        if PAT_WIFI_DOWN.search(line):
            self.events["wifi_down"] += 1

        if PAT_BLE_DOWN.search(line):
            self.events["ble_down"] += 1

        m = PAT_HEAP.search(line)
        if m:
            heap = int(m.group(1))
            if self.heap_samples:
                baseline = statistics.median(self.heap_samples)
                if baseline > 0:
                    drop_pct = 100.0 * (baseline - heap) / baseline
                    if drop_pct >= kHeapDropPctAlert:
                        self.heap_drops.append((ts, heap, drop_pct))
                        self.events["heap_drop"] += 1
                        self._note_alert(
                            f"HEAP-DROP at t+{ts:.1f}s: {heap} bytes ({drop_pct:.1f}% below median {baseline:.0f})"
                        )
            self.heap_samples.append(heap)

    # ─── Summary ─────────────────────────────────────────────────────────────

    def _maybe_summary(self) -> None:
        if time.monotonic() - self.last_summary < kSummaryInterval:
            return
        self._write_summary(final=False)
        self.last_summary = time.monotonic()

    def _write_summary(self, final: bool) -> None:
        elapsed = time.monotonic() - self.start
        heap_min = min(self.heap_samples) if self.heap_samples else None
        heap_med = statistics.median(self.heap_samples) if self.heap_samples else None
        heap_now = self.heap_samples[-1] if self.heap_samples else None

        lines = []
        lines.append(f"# Yui soak summary ({'FINAL' if final else 'rolling'})")
        lines.append("")
        lines.append(f"- Started: {dt.datetime.fromtimestamp(time.time() - elapsed).isoformat(timespec='seconds')}")
        lines.append(f"- Elapsed: {elapsed/3600:.2f} h ({elapsed:.0f} s)")
        lines.append(f"- Target:  {self.duration/3600:.2f} h")
        lines.append(f"- Log:     `{self.log_path.name}`")
        lines.append("")
        lines.append("## Counters")
        lines.append("")
        if self.events:
            for k, v in sorted(self.events.items()):
                lines.append(f"- {k}: {v}")
        else:
            lines.append("- (no events)")
        lines.append("")
        lines.append("## Heap")
        lines.append("")
        if heap_now is not None:
            lines.append(f"- Latest:    {heap_now} bytes")
            lines.append(f"- Median:    {heap_med:.0f} bytes")
            lines.append(f"- Minimum:   {heap_min} bytes")
        else:
            lines.append("- (no heap_free lines parsed)")
        lines.append("")
        lines.append("## Panics / WDTs")
        lines.append("")
        if not self.panics and not self.wdts:
            lines.append("- None.")
        else:
            for ts, ln in self.panics[:20]:
                lines.append(f"- PANIC @ t+{ts:.1f}s — {ln[:200]}")
            for ts, ln in self.wdts[:20]:
                lines.append(f"- WDT   @ t+{ts:.1f}s — {ln[:200]}")
        lines.append("")
        lines.append("## Heap drops")
        lines.append("")
        if not self.heap_drops:
            lines.append("- None.")
        else:
            for ts, heap, pct in self.heap_drops[:20]:
                lines.append(f"- t+{ts:.1f}s — {heap} bytes ({pct:.1f}% below median)")
        lines.append("")

        verdict = "PASS" if not (self.panics or self.wdts or self.boots > kBootLimit or self.heap_drops) else "FAIL"
        lines.append(f"## Verdict: **{verdict}**")
        lines.append("")

        self.summary_path.write_text("\n".join(lines))

    # ─── Helpers ─────────────────────────────────────────────────────────────

    def _note(self, logf, msg: str) -> None:
        print(msg)
        logf.write(f"#####  {msg}\n")

    def _note_alert(self, msg: str) -> None:
        sys.stderr.write(f"!!! {msg}\n")
        sys.stderr.flush()

    def _sigint(self, *_args) -> None:
        print("\n>>> SIGINT — finalizing summary.")
        self._write_summary(final=True)
        sys.exit(130)


# ─── CLI ─────────────────────────────────────────────────────────────────────


def main() -> int:
    p = argparse.ArgumentParser(description="Yui 24h soak-test monitor.")
    p.add_argument("--port", default=os.environ.get("YUI_PORT", "/dev/ttyACM0"))
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--hours", type=float, default=24.0)
    p.add_argument("--outdir", default="release")
    args = p.parse_args()

    monitor = SoakMonitor(
        port=args.port,
        baud=args.baud,
        hours=args.hours,
        outdir=Path(args.outdir),
    )
    return monitor.run()


if __name__ == "__main__":
    sys.exit(main())

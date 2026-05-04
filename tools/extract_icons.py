#!/usr/bin/env python3
"""Slice the menu1 / menu2 source sheets into individual icon PNGs.

The two sheets each carry a 4×4 grid of Japanese-themed icons on a
white background. We detect rows/columns by looking for horizontal and
vertical bands of ink (non-white pixels), then tight-crop each cell
and resize to a square `--size` px output. Resulting files land in
art/icons/<name>_rRcC.png (1-indexed), ready for gen_assets.py to
embed.

Run after dropping a new source sheet in art/icons/_source/. Idempotent.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import sys

from PIL import Image
import numpy as np


def runs(mask: np.ndarray) -> list[tuple[int, int]]:
    """Return [(start, end_exclusive), ...] for runs of True."""
    out = []
    i = 0
    n = len(mask)
    while i < n:
        if mask[i]:
            j = i
            while j < n and mask[j]:
                j += 1
            out.append((i, j))
            i = j
        else:
            i += 1
    return out


def slice_sheet(sheet_path: Path, out_dir: Path, name: str,
                size: int = 64, ink_threshold: int = 250) -> int:
    """Detect grid + emit one PNG per cell. Returns count emitted."""
    img = Image.open(sheet_path).convert("RGBA")
    gray = np.array(img.convert("L"))
    ink = gray < ink_threshold

    rows = runs(ink.any(axis=1))
    cols = runs(ink.any(axis=0))

    if len(rows) == 0 or len(cols) == 0:
        print(f"[icons] {sheet_path.name}: no ink detected — skipping")
        return 0

    print(f"[icons] {sheet_path.name}: detected {len(rows)}×{len(cols)} grid")

    out_dir.mkdir(parents=True, exist_ok=True)
    n = 0
    for ri, (y0, y1) in enumerate(rows, start=1):
        for ci, (x0, x1) in enumerate(cols, start=1):
            # Crop the cell, then re-tighten using the cell's own ink
            # box (some icons sit asymmetrically inside the gross row/
            # col band).
            cell = img.crop((x0, y0, x1, y1))
            cell_gray = np.array(cell.convert("L"))
            cell_ink = cell_gray < ink_threshold
            ys = np.where(cell_ink.any(axis=1))[0]
            xs = np.where(cell_ink.any(axis=0))[0]
            if ys.size == 0 or xs.size == 0:
                continue  # empty cell
            tight = cell.crop((int(xs.min()), int(ys.min()),
                               int(xs.max()) + 1, int(ys.max()) + 1))

            # Pad to square so the icon doesn't distort when scaled.
            w, h = tight.size
            side = max(w, h)
            square = Image.new("RGBA", (side, side), (255, 255, 255, 0))
            square.paste(tight, ((side - w) // 2, (side - h) // 2))

            scaled = square.resize((size, size), Image.LANCZOS)

            # Flatten transparency onto white so PNGs decode fast on
            # the device (no alpha channel needed for chrome-style
            # icons drawn over a known surface).
            flat = Image.new("RGB", scaled.size, (255, 255, 255))
            flat.paste(scaled, mask=scaled.split()[3] if scaled.mode == "RGBA" else None)

            out_path = out_dir / f"{name}_r{ri}c{ci}.png"
            flat.save(out_path, "PNG", optimize=True)
            n += 1
    print(f"[icons] {sheet_path.name}: wrote {n} icons to {out_dir}")
    return n


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=Path(__file__).resolve().parent.parent)
    ap.add_argument("--size", type=int, default=64,
                    help="Target square px for each icon (default: 64)")
    args = ap.parse_args()
    repo = Path(args.repo)
    src_dir = repo / "art" / "icons" / "_source"
    out_dir = repo / "art" / "icons"

    sheets = sorted(src_dir.glob("*.png"))
    if not sheets:
        print(f"[icons] no source sheets in {src_dir}")
        return 1

    total = 0
    for sheet in sheets:
        name = sheet.stem  # 'menu1', 'menu2', etc.
        total += slice_sheet(sheet, out_dir, name, size=args.size)
    print(f"[icons] total: {total} icons emitted at {args.size}×{args.size}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env bash
# verify-lib-extra.sh — sanity-check archives operators dropped into
# board/lib_extra/ before they get linked into the firmware. Reads
# expected SHA-256 hashes from board/lib_extra/EXPECTED_SHA256
# (one per line, format: "<hash>  <filename>"; comments with '#' OK).
#
# Behavior:
#   - Missing archive  → warn, exit 0   (build still works without it)
#   - Hash MATCHES     → exit 0
#   - Hash MISMATCH    → fail, exit 1   (refuse to ship a tampered blob)
#   - File present but no expected hash → warn, exit 1
#
# Run before every release build; wire into release.yml as a gate.
set -euo pipefail

LIB_EXTRA_DIR="$(cd "$(dirname "$0")/.." && pwd)/board/lib_extra"
EXPECTED_FILE="$LIB_EXTRA_DIR/EXPECTED_SHA256"

if [[ ! -d "$LIB_EXTRA_DIR" ]]; then
  echo "verify-lib-extra: $LIB_EXTRA_DIR not found, nothing to verify"
  exit 0
fi

if [[ ! -f "$EXPECTED_FILE" ]]; then
  echo "verify-lib-extra: $EXPECTED_FILE not found, nothing to verify"
  exit 0
fi

fail=0
checked=0

while IFS= read -r line; do
  # Skip blanks and # comments
  [[ -z "${line// }" ]] && continue
  [[ "${line:0:1}" == "#" ]] && continue

  expected_hash="${line%% *}"
  filename="${line##* }"
  archive="$LIB_EXTRA_DIR/$filename"

  if [[ ! -f "$archive" ]]; then
    echo "verify-lib-extra: $filename not present (expected hash $expected_hash) — skipping"
    continue
  fi

  actual_hash="$(sha256sum "$archive" | awk '{print $1}')"
  if [[ "$actual_hash" == "$expected_hash" ]]; then
    echo "verify-lib-extra: $filename OK ($actual_hash)"
    checked=$((checked+1))
  else
    echo "verify-lib-extra: $filename MISMATCH" >&2
    echo "  expected: $expected_hash" >&2
    echo "  actual:   $actual_hash" >&2
    fail=1
  fi
done < "$EXPECTED_FILE"

# Detect archives present but not in EXPECTED_SHA256
for f in "$LIB_EXTRA_DIR"/*.a; do
  [[ -e "$f" ]] || continue
  base="$(basename "$f")"
  if ! grep -qE "[[:space:]]$base$" "$EXPECTED_FILE"; then
    echo "verify-lib-extra: $base present but no expected hash recorded" >&2
    fail=1
  fi
done

if [[ $fail -ne 0 ]]; then
  echo "verify-lib-extra: FAIL — refuse to build with unverified archives" >&2
  exit 1
fi

echo "verify-lib-extra: $checked archive(s) verified"
exit 0

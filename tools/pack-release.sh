#!/usr/bin/env bash
# Build a flashable single-binary release of Yui for the Cardputer ADV.
#
# Output: release/yui-<version>-cardputer_adv.bin
#         release/yui-<version>-manifest.json   (M5Burner descriptor)
#
# Usage:
#   tools/pack-release.sh [VERSION]
#
# Default VERSION = git short hash. The combined .bin starts at offset 0
# and bundles bootloader + partition table + firmware into a single image
# the user can drag-drop into M5Burner's "Custom Firmware" UI or flash via
#   esptool.py --chip esp32s3 write_flash 0x0 yui-X-cardputer_adv.bin

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

VERSION="${1:-$(git rev-parse --short HEAD 2>/dev/null || echo dev)}"
ENV=cardputer_adv

PIO="${PIO:-$(command -v pio || echo $HOME/.platformio/penv/bin/pio)}"

echo ">>> Building $ENV ..."
"$PIO" run -e "$ENV"

BUILD_DIR=".pio/build/$ENV"
BOOTLOADER="$BUILD_DIR/bootloader.bin"
PARTITIONS="$BUILD_DIR/partitions.bin"
FIRMWARE="$BUILD_DIR/firmware.bin"
BOOT_APP="$BUILD_DIR/boot_app0.bin"

# boot_app0.bin lives in the framework — find it.
BOOT_APP_SRC="$(find "$HOME/.platformio/packages/framework-arduinoespressif32" \
                    -name 'boot_app0.bin' 2>/dev/null | head -1 || true)"

mkdir -p release
OUT_BIN="release/yui-${VERSION}-${ENV}.bin"
OUT_MANIFEST="release/yui-${VERSION}-manifest.json"

# Cardputer ADV (ESP32-S3) flash layout:
#   0x0000   bootloader.bin
#   0x8000   partitions.bin
#   0xe000   boot_app0.bin (~8 KB)
#   0x10000  firmware.bin

ESPTOOL="$(find "$HOME/.platformio/packages" -name 'esptool.py' 2>/dev/null | head -1)"
if [ -z "$ESPTOOL" ]; then
  echo "esptool.py not found in PlatformIO packages — falling back to pip"
  ESPTOOL="$(command -v esptool.py)"
fi

# PlatformIO's penv has pyserial; system python may not. Prefer penv.
PENV_PYTHON="$HOME/.platformio/penv/bin/python3"
if [ -x "$PENV_PYTHON" ]; then
  PYTHON="$PENV_PYTHON"
else
  PYTHON="python3"
fi

echo ">>> Merging into $OUT_BIN"
PARTS=(
  --chip esp32s3 merge_bin
  -o "$OUT_BIN"
  --flash_mode qio
  --flash_freq 80m
  --flash_size 8MB
  0x0     "$BOOTLOADER"
  0x8000  "$PARTITIONS"
  0x10000 "$FIRMWARE"
)
if [ -n "$BOOT_APP_SRC" ]; then
  PARTS+=(0xe000 "$BOOT_APP_SRC")
fi

"$PYTHON" "$ESPTOOL" "${PARTS[@]}"

# Write M5Burner-compatible manifest. M5Burner expects a JSON entry
# describing the binary; the format is documented in m5stack/m5burner-list
# but the generic shape is below.
cat > "$OUT_MANIFEST" <<JSON
{
  "name": "Yui",
  "version": "$VERSION",
  "device": "M5Cardputer ADV",
  "chip": "esp32s3",
  "flash_size": "8MB",
  "flash_mode": "qio",
  "flash_freq": "80m",
  "binary": "$(basename "$OUT_BIN")",
  "address": "0x0",
  "description": "Yui — pocket comms + pentest companion. Pairs the Cardputer ADV with a Kenwood TH-D75 (via bb-link bridge), WiFi Pineapple, and uses the onboard radio for native WiFi/BLE work. Includes a satellite tracker that drives the TH-D75 for Doppler shift during a pass.",
  "license": "MIT",
  "homepage": "https://github.com/leingangzj/yui"
}
JSON

echo ""
echo "OK. Outputs:"
ls -la "$OUT_BIN" "$OUT_MANIFEST"
echo ""
echo "Flash via:"
echo "  python3 $ESPTOOL --chip esp32s3 -p /dev/ttyACM0 -b 921600 \\"
echo "    write_flash --flash_mode qio --flash_freq 80m --flash_size 8MB \\"
echo "    0x0 $OUT_BIN"
echo ""
echo "Or drag-drop $OUT_BIN into M5Burner's Custom Firmware slot."

#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT_DIR/tools/esp32s3_jc3248_env.sh"

ERASE_SUFFIX=""
if [[ "${1:-}" == "--erase" ]]; then
  ERASE_SUFFIX=",EraseFlash=all"
fi

FQBN="${ELLERT_JC3248_FQBN}${ERASE_SUFFIX}"

echo "[Ellert] Flash stable display target"
echo "[Ellert] FQBN: $FQBN"
echo "[Ellert] Display port: $ELLERT_DISPLAY_PORT"

arduino-cli compile --fqbn "$FQBN" --export-binaries "$ROOT_DIR/firmware/display_cpu_esp32s3"
arduino-cli upload -p "$ELLERT_DISPLAY_PORT" --fqbn "$FQBN" "$ROOT_DIR/firmware/display_cpu_esp32s3"

echo "[Ellert] Stable display flash complete."

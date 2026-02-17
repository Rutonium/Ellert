#!/usr/bin/env bash
set -euo pipefail

# Shared upload profile for Guition JC3248W535 (AXS15231B QSPI)
# Verified on 2026-02-17
export ELLERT_JC3248_FQBN="esp32:esp32:esp32s3:FlashMode=qio,FlashSize=16M,PartitionScheme=default_8MB,PSRAM=opi,CDCOnBoot=cdc,UploadMode=default,USBMode=hwcdc,CPUFreq=240,LoopCore=1,EventsCore=1"

# Defaults can be overridden by env vars before calling scripts
export ELLERT_INPUT_PORT="${ELLERT_INPUT_PORT:-/dev/ttyACM0}"
export ELLERT_DISPLAY_PORT="${ELLERT_DISPLAY_PORT:-/dev/ttyACM1}"

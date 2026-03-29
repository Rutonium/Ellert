# Ellert

Vehicle control firmware and UI node architecture.

Current architecture:
- `Master CPU`: ESP32 development board (`2A54N-ESP32`) for current integration testing
- `Input CPU`: ESP32-S3 + JC3248W535 display board
- `Display CPU`: ESP32-S3 + JC3248W535 display board

New ESP32 targets now added in parallel:
- `firmware/master_cpu_esp32/` (ESP32 main controller, UART star topology)
- `firmware/input_cpu_esp32s3/` (ESP32-S3 stable runtime input UI, Arduino_GFX path)
- `firmware/display_cpu_esp32s3/` (ESP32-S3 stable runtime display UI, Arduino_GFX path)
- `firmware/display_cpu_esp32s3_lgfx_lab/` (isolated LovyanGFX bring-up lab target)

Active display/input stack for both JC3248 boards:
- `Arduino_GFX` (`Arduino_ESP32QSPI` + `Arduino_AXS15231B` + `Arduino_Canvas`)
- `Wire` reserved for touch controller path (`SDA=4`, `SCL=8`, addr `0x3B`)

## Current Decisions

- Use star topology over UART:
  - Current bring-up mapping in firmware:
    - `Display CPU <-> Master GPIO16/17` (header pins `27/28`)
    - `Input CPU <-> Master GPIO18/19` (header pins `30/31`)
- Display boards are confirmed as `Guition JC3248W535` (AXS15231B QSPI path).
- Runtime build/flash scripts are standardized for JC3248 in `tools/`.
- Current verified runtime state: master reports both nodes online (`input=ON`, `display=ON`) with low heartbeat ages.

## Communication Wiring Standard (Display Boards, Color-Based)

Use this as the canonical wiring reference for both display boards. The displays are encased, so wire color is the trusted identifier:

- `Green` = display `GND`
- `Yellow` = display `RX`
- `Black` = display `TX`
- `Red` = display `+5V`

Master board connection for display link (current mapping):

- `Black` (display `TX`) -> master header pin `27` (`GPIO16`, master `RX`)
- `Yellow` (display `RX`) -> master header pin `28` (`GPIO17`, master `TX`)
- `Green` (display `GND`) -> master `GND` (recommended header pin `32`; `14` or `38` also GND)
- `Red` (display `+5V`) -> master `Vin 5V` header pin `19` (or external 5V supply with common GND)

Master board connection for input display link (current mapping):

- `Black` (input display `TX`) -> master header pin `30` (`GPIO18`, master `RX`)
- `Yellow` (input display `RX`) -> master header pin `31` (`GPIO19`, master `TX`)
- `Green` (input display `GND`) -> master `GND` (recommended header pin `32`; `14` or `38` also GND)
- `Red` (input display `+5V`) -> master `Vin 5V` header pin `19` (or external 5V supply with common GND)

## Power Guidance (Important)

Observed behavior during integration:
- When both display boards were powered from the master ESP32 `5V`, backlight pulsation occurred.
- Pulsation stopped when display USB power was connected, indicating power-path/current limitation.

Recommended runtime power topology:
- Use a dedicated, stable external `5V` supply for both display boards.
- Keep UART wiring unchanged.
- Keep a common ground between:
  - external `5V` supply
  - master ESP32
  - both display boards

Current planning target:
- Budget at least `2A` total for both display boards combined (more headroom preferred).

## Process Status

Implemented:
- 3-CPU protocol skeleton and heartbeat/status framing
- Input touch grid (16 command buttons)
- Master-side remote input parsing and status publishing
- Master pin remap with user-panel inputs marked free/unassigned
- Display dashboard layout with:
  - centered speed/power headers
  - improved rounded speed font rendering
  - gear indicator inside speed box
  - indicator demo blink (`0.7s ON / 0.7s OFF`) with yellow active-circle
  - top-left time and `km/t` speed unit

Next session:
1. Fine-tune dashboard/input UX now that UART links are stable.
2. Replace placeholder telemetry in `master_cpu_esp32` with real motor/BMS data.
3. Remove temporary link-debug presentation once final UI is locked.

## Start Here

For complete, verified setup and deployment details, read:

- `CODEX_TRUTH.md`

That file is the single source of truth for:
- cold-start environment setup
- required toolchain and library versions
- exact compile/upload commands
- known issues and fixes

## Quick Commands

From repo root:

```bash
arduino-cli compile --fqbn arduino:sam:arduino_due_x_dbg --export-binaries firmware/master_cpu
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:sam:arduino_due_x_dbg --input-dir firmware/master_cpu/build/arduino.sam.arduino_due_x_dbg firmware/master_cpu
```

JC3248 ESP32-S3 UI nodes (Input + Display):

```bash
./tools/compile_jc3248_nodes.sh
./tools/flash_jc3248_nodes.sh
```

With full flash erase:

```bash
./tools/flash_jc3248_nodes.sh --erase
```

Display-only fail-safe commands:

```bash
# Flash known-good stable display firmware (recommended runtime target)
./tools/flash_display_stable.sh

# Flash isolated LGFX lab target (diagnostics/bring-up only)
./tools/flash_display_lgfx_lab.sh
```

## Repository Layout

- `firmware/master_cpu/` - active Arduino Due firmware
- `firmware/input_cpu/` - temporary UNO touch input firmware
- `firmware/display_cpu/` - temporary UNO display firmware
- `firmware/master_cpu_esp32/` - new ESP32 master target
- `firmware/input_cpu_esp32s3/` - stable ESP32-S3 runtime input target
- `firmware/display_cpu_esp32s3/` - stable ESP32-S3 runtime display target
- `firmware/display_cpu_esp32s3_lgfx_lab/` - isolated LovyanGFX display lab target
- `firmware/display_cpu_esp32s3_lvgl/` - experimental LVGL path (not runtime target)
- `shared/protocol/` - cross-CPU message definitions
- `docs/` - active architecture notes
- `JC3248W535EN_Touch_LCD-0.9.5/` - local display/touch reference package added to repo
- `old_cleanded/` - archived legacy/vendor/IDE files moved out of active tree

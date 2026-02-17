# Ellert

Vehicle control firmware and UI node architecture.

Current architecture:
- `Master CPU`: ESP32 development board (`2A54N-ESP32`) for current integration testing
- `Input CPU`: ESP32-S3 + JC3248W535 display board
- `Display CPU`: ESP32-S3 + JC3248W535 display board

New ESP32 targets now added in parallel:
- `firmware/master_cpu_esp32/` (ESP32 main controller, UART star topology)
- `firmware/input_cpu_esp32s3/` (ESP32-S3 LVGL + TFT_eSPI input UI)
- `firmware/display_cpu_esp32s3/` (ESP32-S3 LVGL + TFT_eSPI display UI)

## Current Decisions

- Use star topology over UART:
  - `Input CPU <-> Master UART1 (GPIO16/17)`
  - `Display CPU <-> Master UART2 (GPIO18/19)`
- Display boards are confirmed as `Guition JC3248W535` (AXS15231B QSPI path).
- Runtime build/flash scripts are standardized for JC3248 in `tools/`.
- Remaining blocker before full 3-board integration: correct physical UART cables for display boards (`JST Micro 1.25`).

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
1. Connect Master/Input/Display over UART using new JST 1.25 cables.
2. Validate end-to-end command flow: touch press -> master action -> dashboard update.
3. Replace placeholder telemetry in `master_cpu_esp32` with real motor/BMS data.

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

## Repository Layout

- `firmware/master_cpu/` - active Arduino Due firmware
- `firmware/input_cpu/` - temporary UNO touch input firmware
- `firmware/display_cpu/` - temporary UNO display firmware
- `firmware/master_cpu_esp32/` - new ESP32 master target
- `firmware/input_cpu_esp32s3/` - new ESP32-S3 LVGL input target
- `firmware/display_cpu_esp32s3/` - new ESP32-S3 LVGL display target
- `shared/protocol/` - cross-CPU message definitions
- `docs/` - active architecture notes
- `JC3248W535EN_Touch_LCD-0.9.5/` - local display/touch reference package added to repo
- `old_cleanded/` - archived legacy/vendor/IDE files moved out of active tree

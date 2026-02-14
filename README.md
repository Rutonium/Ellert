# Ellert

Vehicle control firmware and UI node architecture.

Current architecture:
- `Master CPU`: Arduino Due fail-safe control hub
- `Input CPU`: temporary Arduino UNO R3 + 3.5" touch (to be replaced)
- `Display CPU`: temporary Arduino UNO R3 + 3.5" display (to be replaced)

## Current Decisions

- Keep `Master CPU` on Arduino Due for now.
- Keep star topology:
  - `Input CPU <-> Due Serial1`
  - `Display CPU <-> Due Serial2`
- User panel physical inputs moved off master GPIO and into `Input CPU` command frames.
- `Display CPU` dashboard implemented and running on UNO + 3.5" display as an interim step.
- Planned hardware migration:
  - Replace UI UNOs + screens with ESP32-S3 integrated 3.5" boards (JC3248W535/EN class).
  - Keep master fail-safe controller separate.

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
1. Port `Input CPU` and `Display CPU` firmware from UNO libraries to ESP32-S3 stack.
2. Replace display demo values with full live telemetry fields.
3. Tune final visual design and performance for the new display boards.

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

## Repository Layout

- `firmware/master_cpu/` - active Arduino Due firmware
- `firmware/input_cpu/` - temporary UNO touch input firmware
- `firmware/display_cpu/` - temporary UNO display firmware
- `shared/protocol/` - cross-CPU message definitions
- `docs/` - active architecture notes
- `old_cleanded/` - archived legacy/vendor/IDE files moved out of active tree

# Ellert

Vehicle control firmware and UI node architecture.

Current architecture:
- `Master CPU`: Arduino Due fail-safe control hub
- `Input CPU`: Arduino UNO R3 touch/button user input node
- `Display CPU`: dedicated display node (read-only from control perspective)

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
- `firmware/input_cpu/` - UNO touch input firmware area
- `firmware/display_cpu/` - display firmware area
- `shared/protocol/` - cross-CPU message definitions
- `docs/` - hardware docs, vendor libraries, IO and archives

# Ellert Codex Truth (Linux Mint + Arduino Due)

This document is the single source of truth for getting this project working from a cold start.

## Project Identity

- Project root: `/home/rune/Documents/VS Code repos/Ellert`
- Master firmware folder: `firmware/master_cpu/`
- Target board: **Arduino Due (Programming Port)**
- Working FQBN: `arduino:sam:arduino_due_x_dbg`
- Typical upload port: `/dev/ttyACM0`

## Documentation Discipline (Mandatory)

Whenever a pin mapping, board behavior, dependency, build parameter, or deployment workflow is newly confirmed:

- Update `CODEX_TRUTH.md` in the same change set.
- Update `INPUT_OUTPUT_MAP.md` in the same change set if any input/output mapping changed.
- Treat these two files as required maintenance artifacts, not optional notes.

## Known-Good Current State

- `arduino-cli` installed and working:
  - `arduino-cli 1.4.1` (seen on this machine)
- Arduino core installed:
  - `arduino:sam 1.6.12`
- Required libraries installed:
  - `Adafruit GFX Library 1.12.4`
  - `Adafruit BusIO 1.17.4`
  - `Adafruit RA8875 1.4.4`
- EEPROM compatibility shim added in project:
  - `firmware/master_cpu/EEPROM.h`
  - `firmware/master_cpu/EEPROM.cpp`
  - `firmware/master_cpu/PinMap.cpp` uses `#include "EEPROM.h"`
  - `firmware/master_cpu/PinRules.cpp` uses `#include "EEPROM.h"`
- Headless/No-screen behavior:
  - `DisplayManager` no longer blocks boot if RA8875 is missing.
  - Serial diagnostics stream is enabled for PC-side mock display.
- Canonical pin map document:
  - `INPUT_OUTPUT_MAP.md`
- Latest confirmed change: **position-light toggle input uses `D52`** (`PIN_LIGHTS_STALK_PARKING = 52`), with `PIN_FAN_MID` moved to `D33`.
- Latest naming discovery:
  - Panel-input prefix requested: `trykknap-panel`
  - Applied to matching configured input labels in `PinMap::digitalInputLabel`
  - User-provided pin list recorded in `INPUT_OUTPUT_MAP.md` with input/output status for each pin.

## Why EEPROM Shim Exists

`arduino:sam` (Due) does not provide `<EEPROM.h>` the same way AVR projects do.  
This project now uses a local compatibility layer (`"EEPROM.h"`) so compile/upload works on Due.

Important behavior:
- The shim is RAM-backed.
- Data is **not persistent across reset/reflash**.
- If true persistence is needed later, migrate to Due flash-backed storage.

## Exact Cold-Start Setup

Run from terminal.

1. Install `arduino-cli` (if missing):
```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
sudo install -m 755 bin/arduino-cli /usr/local/bin/arduino-cli
hash -r
arduino-cli version
```

2. Install board core + libraries:
```bash
arduino-cli core update-index
arduino-cli core install arduino:sam
arduino-cli lib install "Adafruit GFX Library" "Adafruit RA8875" "Adafruit BusIO"
```

3. Ensure serial port permissions:
```bash
sudo usermod -aG dialout $USER
```
Then log out/in once (or run `newgrp dialout` in current shell).

4. Verify board appears:
```bash
arduino-cli board list
```
Expected board line should show:
- Port: `/dev/ttyACM0` (or similar)
- Board Name: `Arduino Due (Programming Port)`
- FQBN: `arduino:sam:arduino_due_x_dbg`

## Exact Build + Upload (Known-Good)

From project root:

```bash
cd "/home/rune/Documents/VS Code repos/Ellert"
arduino-cli compile --fqbn arduino:sam:arduino_due_x_dbg --export-binaries firmware/master_cpu
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:sam:arduino_due_x_dbg --input-dir firmware/master_cpu/build/arduino.sam.arduino_due_x_dbg firmware/master_cpu
```

## Headless Mock Display (No Physical Screen)

Firmware publishes periodic diagnostics over serial:

- `DIAG|D|v0,v1,...,v65|A|a0,a1,...,a11`

PC mock display script:

```bash
python3 tools/mock_display_serial.py --port /dev/ttyACM0 --baud 115200
```

If needed, verify raw stream first:

```bash
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200
```

## Fast Troubleshooting

### 1) `Cannot perform port reset: 1200-bps touch ... Permission denied`

Cause: user not in `dialout` group or group not applied yet.

Fix:
```bash
sudo usermod -aG dialout $USER
newgrp dialout
```
Then retry upload.

### 2) `No such file or directory` during upload after erase starts

Cause: build artifacts missing or built with different FQBN.

Fix:
- Recompile with `--export-binaries` and same FQBN as upload.
- Use `--input-dir firmware/master_cpu/build/arduino.sam.arduino_due_x_dbg`.

### 3) `fatal error: EEPROM.h: No such file or directory`

Cause: wrong include style or missing local shim.

Fix:
- Ensure `firmware/master_cpu/EEPROM.h` and `firmware/master_cpu/EEPROM.cpp` exist.
- Ensure include is quoted in both files:
  - `firmware/master_cpu/PinMap.cpp`: `#include "EEPROM.h"`
  - `firmware/master_cpu/PinRules.cpp`: `#include "EEPROM.h"`

### 4) `No boards found`

Fix checklist:
- Confirm USB cable supports data (not charge-only).
- Replug board, press reset once.
- Re-run:
```bash
arduino-cli board list
```

### 5) Port busy

Close anything holding `/dev/ttyACM0` (Serial Monitor, IDE, `screen`, `minicom`), then retry upload.

## VS Code Extensions (Recommended)

- `ms-vscode.cpptools`
- `vscode-arduino.vscode-arduino-community`
- `ms-vscode.powershell`
- `ms-dotnettools.csdevkit`
- `ms-dotnettools.csharp`

## Notes About This Environment

- `git` command may not be installed in this shell environment.
- `ripgrep` (`rg`) may not be installed by default.
- `mock_display` targets `net6.0-windows` and is Windows-only unless adapted.

## Multi-CPU Direction (Current Project Decision)

- `Master CPU` (Arduino Due) remains mandatory fail-safe controller.
- `Input CPU` and `Display CPU` are optional runtime nodes connected over serial links.
- Vehicle must continue operating in safe mode with `Master CPU` only.

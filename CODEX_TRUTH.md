# Ellert Codex Truth (Linux Mint + Arduino Due)

This document is the single source of truth for getting this project working from a cold start.

## Project Identity

- Project root: `/home/rune/Documents/VS Code repos/Ellert`
- Master firmware folder: `firmware/master_cpu/`
- Target board: **Arduino Due (Programming Port)**
- Working FQBN: `arduino:sam:arduino_due_x_dbg`
- Typical upload port: `/dev/ttyACM0`

Current integration status:
- Both ESP32-S3 JC3248 boards are confirmed working with intended UIs when flashed separately.
- Physical 3-board UART integration is pending cable delivery (`JST Micro 1.25`).

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
- Latest architecture mapping update:
  - Master user-panel GPIO inputs moved to `Input CPU` (set to unassigned `-1` in `PinDefinitions.h`).
  - Ventilation outputs remapped to `D49`, `D50`, `D51`.
  - Functional output mapping documented in `INPUT_OUTPUT_MAP.md`.
- Latest display CPU update:
  - `Display CPU` now targets Arduino UNO + 3.5" TFT (same hardware class as `Input CPU`).
  - Dashboard layout implemented in `firmware/display_cpu/display_cpu.ino` in landscape orientation.
  - Master status payload expanded for dashboard data in `RemoteInterfaces.cpp` / `protocol_v1.h`.
  - Dashboard visual updates added: centered `SPEED/POWER` headers, smoother speed font, gear indicator in speed box, blinking indicator circles, top-left time, `km/t` unit.
- Current hardware decision:
  - `Master CPU` remains Arduino Due for now.
  - UI migration target is ESP32-S3 integrated 3.5" boards (JC3248W535/EN class) for both `Input CPU` and `Display CPU`.
  - Existing UNO-based input/display firmware is interim and will be ported when boards arrive.
- New ESP32 firmware targets created and compiling:
  - `firmware/master_cpu_esp32/` (FQBN `esp32:esp32:esp32`)
  - `firmware/input_cpu_esp32s3/` (FQBN `esp32:esp32:esp32s3`)
  - `firmware/display_cpu_esp32s3/` (FQBN `esp32:esp32:esp32s3`)
- Added libraries for ESP32 UI stack:
  - `lvgl 9.4.0`
  - `TFT_eSPI 2.5.43`
- Added troubleshooting libraries (installed locally):
  - `TCA9554 0.1.2`
  - `XPowersLib 0.2.9`
  - `JPEGDecoder 2.0.0`

## ESP32-S3 JC3248W535 Truth (Verified 2026-02-17)

This section is now authoritative for the two display boards.

- Physical board ID (silkscreen, user-confirmed): **Guition `JC3248W535`**
- Panel path: **AXS15231B over QSPI** (not Waveshare ST7796 SPI path)
- Backlight pin: `GPIO1`
- QSPI pins:
  - `CS=45`
  - `SCK=47`
  - `D0=21`
  - `D1=48`
  - `D2=40`
  - `D3=39`
- Touch controller bus (configured/known):
  - `SDA=4`
  - `SCL=8`
  - `RST=12`
  - `INT=11`
  - I2C address `0x3B`

### Known Device Ports (Current Machine)

- Display board #1: `/dev/ttyACM0` (MAC `e8:f6:0a:8b:9b:44`)
- Display board #2: `/dev/ttyACM1` (MAC `e8:f6:0a:8b:9b:4c`)
- Main ESP32 board (non-display): `/dev/ttyUSB0`

### Session Stop Point (2026-02-17)

- Verified: both JC3248 screens flash correctly and run:
  - `firmware/input_cpu_esp32s3`
  - `firmware/display_cpu_esp32s3`
- Verified: runtime scripts created and working:
  - `tools/esp32s3_jc3248_env.sh`
  - `tools/compile_jc3248_nodes.sh`
  - `tools/flash_jc3248_nodes.sh`
- Pending: physical UART connection between master and both display boards due to missing JST 1.25 cables.

### Required ESP32 Upload Profile (Critical)

Use this exact profile for JC3248W535 display boards:

- Board: `ESP32S3 Dev Module` (`esp32:esp32:esp32s3`)
- `FlashMode=qio`
- `FlashSize=16M`
- `PartitionScheme=default_8MB`
- `PSRAM=opi`
- `CDCOnBoot=cdc`
- `UploadMode=default`
- `USBMode=hwcdc`
- `CPUFreq=240`
- `LoopCore=1`
- `EventsCore=1`

If boards are in a bad state, include:
- `EraseFlash=all`

Exact FQBN string:

```bash
esp32:esp32:esp32s3:FlashMode=qio,FlashSize=16M,PartitionScheme=default_8MB,PSRAM=opi,CDCOnBoot=cdc,UploadMode=default,USBMode=hwcdc,CPUFreq=240,LoopCore=1,EventsCore=1,EraseFlash=all
```

### Verified Working Probe Sketch

Path used during recovery:
- `/tmp/jc3248_truth_probe/jc3248_truth_probe.ino`

Behavior when working:
- full-screen color cycling (`RED/GREEN/BLUE/WHITE/BLACK`)
- readable text (`JC3248W535`, `AXS15231B`, color name)

### Fast Recovery Procedure (Display Boards)

1. Flash a backlight rescue sketch if panel appears dead:
```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3 /tmp/bl_rescue
arduino-cli upload -p /dev/ttyACM0 --fqbn esp32:esp32:esp32s3 /tmp/bl_rescue
```
2. Reflash JC3248 truth probe with full FQBN options and erase:
```bash
arduino-cli compile --fqbn 'esp32:esp32:esp32s3:FlashMode=qio,FlashSize=16M,PartitionScheme=default_8MB,PSRAM=opi,CDCOnBoot=cdc,UploadMode=default,USBMode=hwcdc,CPUFreq=240,LoopCore=1,EventsCore=1,EraseFlash=all' --libraries "$HOME/Arduino/libraries" /tmp/jc3248_truth_probe
arduino-cli upload -p /dev/ttyACM0 --fqbn 'esp32:esp32:esp32s3:FlashMode=qio,FlashSize=16M,PartitionScheme=default_8MB,PSRAM=opi,CDCOnBoot=cdc,UploadMode=default,USBMode=hwcdc,CPUFreq=240,LoopCore=1,EventsCore=1,EraseFlash=all' /tmp/jc3248_truth_probe
```
3. Repeat on `/dev/ttyACM1`.

### Driver / Reference Locations Used

In-repo runtime configs:
- `firmware/input_cpu_esp32s3/board_config.h`
- `firmware/display_cpu_esp32s3/board_config.h`

Local installed Arduino libraries:
- `$HOME/Arduino/libraries/GFX_Library_for_Arduino`
- `$HOME/Arduino/libraries/JC3248W535EN-Touch-LCD`
- `$HOME/Arduino/libraries/TCA9554`
- `$HOME/Arduino/libraries/XPowersLib`

Downloaded external references used during debugging:
- `/tmp/jc_test/JC3248W535_display_test-main` (community JC3248 test)
- `/tmp/jc_audun/JC3248W535EN-Touch-LCD-main` (Audun library source)
- `/tmp/waveshare35` (Waveshare package; useful for contrast only, wrong board family for final setup)

In-repo display/touch reference bundle (added by user):
- `JC3248W535EN_Touch_LCD-0.9.5/`

### Important Non-Truth (Do Not Repeat)

- Do **not** treat this board as Waveshare ESP32-S3 Touch LCD 3.5 for display-driver setup.
- Do **not** use ST7796 SPI (`DC=3, SCLK=5, MOSI=1`) as the final board path for this project.
- For this project, final display truth is **JC3248W535 + AXS15231B + QSPI pin set above**.

## 2A54N-ESP32 Master Board Header Mapping (Verified from `2A54N-ESP32.pdf`)

Important: firmware uses **GPIO numbers**, not physical header numbers.

For Master UART links in `firmware/master_cpu_esp32/master_cpu_esp32.ino`:
- Input CPU UART1: `RX=GPIO16`, `TX=GPIO17`
- Display CPU UART2: `RX=GPIO18`, `TX=GPIO19`

On the 2A54N board header numbering (from the PDF pin diagram):
- Physical pin `27` = `GPIO16`
- Physical pin `28` = `GPIO17`
- Physical pin `30` = `GPIO18`
- Physical pin `31` = `GPIO19`

Do not confuse with:
- Physical pin `18` = `GPIO11` (not UART link pin)
- Physical pin `19` = `Vin 5V` (power pin)

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
arduino-cli core install esp32:esp32
arduino-cli lib install lvgl TFT_eSPI
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

ESP32 targets:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 --export-binaries firmware/master_cpu_esp32
arduino-cli compile --fqbn esp32:esp32:esp32s3 --export-binaries firmware/input_cpu_esp32s3
arduino-cli compile --fqbn esp32:esp32:esp32s3 --export-binaries firmware/display_cpu_esp32s3
```

Default upload examples:

```bash
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 firmware/master_cpu_esp32
arduino-cli upload -p /dev/ttyACM0 --fqbn esp32:esp32:esp32s3 firmware/input_cpu_esp32s3
arduino-cli upload -p /dev/ttyACM1 --fqbn esp32:esp32:esp32s3 firmware/display_cpu_esp32s3
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

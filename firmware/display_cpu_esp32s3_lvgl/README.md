# Display CPU ESP32-S3 LVGL (Experimental Scaffold)

Bootable LVGL scaffold for Guition JC3248W535 (AXS15231B QSPI).

Status:
- Experimental only.
- Not the runtime firmware path used for vehicle integration.
- Runtime display target remains `firmware/display_cpu_esp32s3/` (Arduino_GFX stack).

Current scope:
- Minimal LVGL boot screen
- Master heartbeat + status snapshot decode
- Diagnostics screen toggle by press-and-hold in lower-left corner (~1.2s)
- Diagnostics fields for comms health, active output bits, and input event log
- Display backend migrated to LovyanGFX (IceNav-v3 style architecture)

## Test screen gesture

- On display: press and hold lower-left corner to enter diagnostics.
- Repeat same hold gesture to return to main screen.

## Default serial link

- Master link: `UART1 RX=44 TX=43` (set in `board_config.h`)

## Compile

```bash
arduino-cli lib install "LovyanGFX"
arduino-cli compile --fqbn 'esp32:esp32:esp32s3:FlashMode=qio,FlashSize=16M,PartitionScheme=default_8MB,PSRAM=opi,CDCOnBoot=cdc,UploadMode=default,USBMode=hwcdc,CPUFreq=240,LoopCore=1,EventsCore=1' --export-binaries firmware/display_cpu_esp32s3_lvgl
```

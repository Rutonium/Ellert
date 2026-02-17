# Input CPU ESP32-S3 (JC3248W535)

ESP32-S3 input node for Guition JC3248W535 (AXS15231B QSPI).

Current scope:
- 4x4 command grid (16 buttons)
- Protocol-compatible heartbeat + input event frames
- UART link to master

## Required libraries

```bash
arduino-cli lib install "GFX Library for Arduino"
```

Display path in this target is `Arduino_GFX` + `Arduino_AXS15231B` + `Arduino_Canvas`.

## Default serial link

- Master link: `UART1 RX=44 TX=43` (adjust in `board_config.h`)

## Compile + Upload (recommended)

```bash
./tools/compile_jc3248_nodes.sh
./tools/flash_jc3248_nodes.sh
```

To erase flash before upload:

```bash
./tools/flash_jc3248_nodes.sh --erase
```

# Display CPU ESP32-S3 (JC3248W535)

ESP32-S3 display node for Guition JC3248W535 (AXS15231B QSPI).

Current scope:
- Horizontal dashboard layout
- Runtime map screen (OSM tile rendering + vehicle marker + telemetry HUD)
- Protocol heartbeat + status snapshot decoding
- Indicator blink behavior and key telemetry cards

## Required libraries

```bash
arduino-cli lib install "GFX Library for Arduino"
arduino-cli lib install "PNGdec"
```

Display path in this target is `Arduino_GFX` + `Arduino_AXS15231B` + `Arduino_Canvas`.

Map scaffold note:
- `kShowMapScreen` in `display_cpu_esp32s3.ino` controls whether map scaffold is shown by default.
- Map now follows GPS coordinates from master status payload:
  - `latE7` / `lonE7`
  - `headingCdeg`
  - `gpsFix` / `gpsSats`
- Runtime tile behavior:
  - Display uses local Wi-Fi credentials (`wifi_local.h`, git-ignored)
  - Fetches OpenStreetMap PNG tiles (`z/x/y`)
  - Decodes tiles with `PNGdec`
  - Renders a 3x3 neighborhood around the GPS tile center
  - Stores fetched tiles in `LittleFS` cache for reuse across reboots
  - Uses non-destructive mount (`LittleFS.begin(false)`) to avoid cache wipe on normal flashes
  - Fetches at limited cadence (`~1 tile / 1.2s`) to reduce provider load
  - Shows attribution footer `(c)OSM` on map screen
  - Current style default: grayscale tiles
  - Current default zoom: `17` (can be changed from master with `MAPZOOM <0..19>`)

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

# Master CPU ESP32

ESP32 port of the fail-safe `Master CPU`.

Current scope:
- ESP32-compatible pin map for required outputs
- Two serial links (star topology):
  - `Input CPU` on UART1
  - `Display CPU` on UART2
- Same framed protocol as `shared/protocol/protocol_v1.h`

## Default pin plan (adjust as needed)

- Input CPU link (currently `HardwareSerial(1)`): `RX=18`, `TX=19` (header pins `30/31`)
- Display CPU link (currently `HardwareSerial(2)`): `RX=16`, `TX=17` (header pins `27/28`)

Outputs:
- `GPIO2` Daytime running lights
- `GPIO4` Front near
- `GPIO5` Front high beam
- `GPIO21` Indicator left
- `GPIO22` Indicator right
- `GPIO23` Brake light
- `GPIO25` Horn
- `GPIO26` Sprinkler
- `GPIO27` Wiper intermittent
- `GPIO14` Wiper normal
- `GPIO13` Wiper fast
- `GPIO12` Ventilation low
- `GPIO15` Ventilation mid
- `GPIO33` Ventilation high

Inputs:
- `GPIO32` Ignition switch
- `GPIO35` Brake pedal
- `GPIO34` Pedal analog (ADC)

## Compile (example)

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 --export-binaries firmware/master_cpu_esp32
```

## GPS Input Bridge (USB Serial)

Master accepts GPS coordinate injection from USB serial and forwards it to the display in `MSG_STATUS_SNAPSHOT`.

Commands:

```text
GPS <lat> <lon> [headingDeg] [sats]
GPS?
MAPZOOM <0..19>
NTP?
TILETEST <z> <x> <y>
GPSHELP
```

Example:

```text
GPS 55.6761 12.5683 92 9
```

Notes:
- NMEA RMC lines (`$GPRMC` / `$GNRMC`) are also accepted on USB serial.
- Display map view follows these coordinates when `gpsFix=1`.
- `NTP?` prints current NTP/local time sync status on master.
- `TILETEST` performs a single online map-tile fetch prototype over Wi-Fi and prints HTTP + byte count.
- `MAPZOOM` sets map zoom broadcast to display (`0..19`).
- Default map zoom at boot is `17`.
- Master seeds a boot GPS fix (`55.527450, 8.470522`) so map rendering starts immediately after reboot/flash.

Input-display zoom mapping (command path):
- `MAP -` button (`CMD_FAN_LOW`) decreases map zoom.
- `MAP +` button (`CMD_FAN_HIGH`) increases map zoom.

## Wi-Fi Local-Only Config

This target supports optional Wi-Fi connection using a local secrets file:

1. Copy `wifi_local.example.h` to `wifi_local.h`
2. Set:
   - `kWifiSsid`
   - `kWifiPassword`
3. Flash master firmware

Security:
- `wifi_local.h` is git-ignored and stays local on your machine.

Serial status messages:
- `WIFI_CONNECT ssid=...`
- `WIFI_OK ip=...`
- `WIFI_WAIT status=<code>`

# Master CPU ESP32

ESP32 port of the fail-safe `Master CPU`.

Current scope:
- ESP32-compatible pin map for required outputs
- Two serial links (star topology):
  - `Input CPU` on UART1
  - `Display CPU` on UART2
- Same framed protocol as `shared/protocol/protocol_v1.h`

## Default pin plan (adjust as needed)

- UART1 Input CPU: `RX=16`, `TX=17`
- UART2 Display CPU: `RX=18`, `TX=19`

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

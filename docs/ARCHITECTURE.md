# Ellert Multi-CPU Architecture

## Roles

- `Master CPU` (Arduino Due): mandatory, fail-safe, controls all vehicle outputs.
- `Input CPU` (Arduino UNO + touch UI): optional, sends user intent only.
- `Display CPU`: optional, receives status snapshots and renders UI.

## Wiring (Simple Star Topology)

- `Input CPU` UART -> `Master CPU` `Serial1`
- `Display CPU` UART -> `Master CPU` `Serial2`
- Common GND across all boards

Electrical note:
- Arduino Due RX is 3.3V tolerant.
- Any 5V TX line into Due RX (for example UNO TX) must use level shifting.

## Fail-Safe Rules

- `Master CPU` owns all safety logic and actuator decisions.
- `Input CPU` never controls relays, PWM, or direct outputs.
- `Display CPU` is strictly read-only.
- Each optional CPU must send periodic heartbeat frames.
- On heartbeat timeout or malformed messages, `Master CPU` ignores remote data and continues safe operation.

## Protocol Baseline

Shared header location:
- `shared/protocol/protocol_v1.h`

Current frame concept:
- `SOF | version | type | seq | len | payload | crc8`

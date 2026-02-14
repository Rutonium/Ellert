# Input CPU 4x4 Command Grid

The touchscreen grid sends command IDs `0..15` in row-major order.

1. `0` -> `LIGHTS_OFF`
2. `1` -> `LIGHTS_PARK`
3. `2` -> `LIGHTS_LOW`
4. `3` -> `LIGHTS_HIGH`
5. `4` -> `IND_LEFT`
6. `5` -> `IND_RIGHT`
7. `6` -> `HAZARD`
8. `7` -> `HORN`
9. `8` -> `WIPER_INT`
10. `9` -> `WIPER_LOW`
11. `10` -> `WIPER_HIGH`
12. `11` -> `WASHER`
13. `12` -> `FAN_LOW`
14. `13` -> `FAN_MID`
15. `14` -> `FAN_HIGH`
16. `15` -> `DEMIST`

Message payload format for `MSG_INPUT_STATE`:

1. byte0: flags
2. byte1: pressed mask low byte
3. byte2: pressed mask high byte
4. byte3: event command id (`0..15`, or `0xFF` for periodic state frame)
5. byte4: event type (`0=none`, `1=press`, `2=release`)

# Ellert Input/Output Map (Canonical)

Source of truth: `firmware/master_cpu/PinDefinitions.h` (used by `PinMap::applyDefaults()`).

## Inputs

- `PIN_ACCELERATOR = A0`
- `PIN_IGNITION_ON = 22`
- `PIN_IGNITION_ACC = 23`
- `PIN_IGNITION_START = 24`
- `PIN_BRAKE_PEDAL = 25`
- `PIN_HANDBRAKE = 26`
- `PIN_GEAR_PARK = 27`
- `PIN_GEAR_REVERSE = 28`
- `PIN_GEAR_NEUTRAL = 29`
- `PIN_GEAR_DRIVE = 30`
- `PIN_IND_STALK_LEFT = 31`
- `PIN_IND_STALK_RIGHT = 32`
- `PIN_LIGHTS_STALK_PARKING = 52` (position light toggle button)
- `PIN_LIGHTS_STALK_HEAD = 34`
- `PIN_HIGH_BEAM_STALK = 35`
- `PIN_HAZARD_SWITCH = 36`
- `PIN_WIPER_STALK_LOW = 37`
- `PIN_WIPER_STALK_HIGH = 38`
- `PIN_WIPER_STALK_INT = 39`
- `PIN_WASHER_SWITCH = 40`
- `PIN_HORN_BUTTON = 41`
- `PIN_DEMISTER_SWITCH = 42`

## Inputs Moved To Input CPU (Master Unassigned / Free)

- `PIN_TRIP_RESET_BUTTON = -1` (free pin `D48`)
- `PIN_HEAT_ELEM_1_SW = -1` (free pin `D49`)
- `PIN_HEAT_ELEM_2_SW = -1` (free pin `D50`)
- `PIN_FAN_LOW = -1` (free pin `D51`)
- `PIN_FAN_MID = -1` (free pin `D33`)
- `PIN_FAN_HIGH = -1` (free pin `D53`)
- `PIN_AC_OFF_ALL = -1` (free pin `D55`)
- `PIN_AC_VENT_ONLY = -1` (free pin `D56`)
- `PIN_AC_HEAT_1 = -1` (free pin `D57`)
- `PIN_AC_HEAT_2 = -1` (free pin `D58`)
- `PIN_WIPER_START_USER = -1` (free pin `D59`)
- `PIN_WIPER_STOP_USER = -1` (free pin `D60`)
- `PIN_SPRINKLER_USER = -1` (free pin `D61`)
- `PIN_LIGHTS_NORMAL_USER = -1` (free pin `D62`)
- `PIN_LIGHTS_HIGH_USER = -1` (free pin `D63`)
- `PIN_LIGHTS_OFF_USER = -1` (free pin `D64`)

## Outputs

- `PIN_MOTOR_PWM_OUT = DAC0`
- `PIN_MAIN_CONTACTOR = 2`
- `PIN_BRAKE_LIGHTS_RELAY = 4`
- `PIN_HEADLIGHTS_RELAY = 6`
- `PIN_HIGH_BEAMS_RELAY = 7`
- `PIN_PARKING_LIGHTS_RELAY = 8`
- `PIN_IND_LEFT_RELAY = 14`
- `PIN_IND_RIGHT_RELAY = 15`
- `PIN_HORN_RELAY = 16`
- `PIN_REVERSE_LIGHT_RELAY = 17`
- `PIN_P_BRAKE_INDICATOR = 18`
- `PIN_INTERIOR_LIGHT_RELAY = 19`
- `PIN_CHARGING_RELAY = 43`
- `PIN_WIPER_INT_RELAY = 44`
- `PIN_WIPER_LOW_RELAY = 45`
- `PIN_WIPER_HIGH_RELAY = 46`
- `PIN_WASHER_PUMP_RELAY = 47`
- `PIN_HEATER_ELEM_1_RELAY = 49`
- `PIN_HEATER_ELEM_2_RELAY = 50`
- `PIN_HEATER_FAN_RELAY = 51`
- `PIN_DEMISTER_RELAY = 5`

## Functional Output Mapping (Requested)

- `Daytime running lights` -> `PIN_PARKING_LIGHTS_RELAY (D8)`
- `Front Near` -> `PIN_HEADLIGHTS_RELAY (D6)`
- `Front High Beam` -> `PIN_HIGH_BEAMS_RELAY (D7)`
- `Indicator Left` -> `PIN_IND_LEFT_RELAY (D14)`
- `Indicator Right` -> `PIN_IND_RIGHT_RELAY (D15)`
- `Brake light` -> `PIN_BRAKE_LIGHTS_RELAY (D4)`
- `Horn` -> `PIN_HORN_RELAY (D16)`
- `Sprinkler On` -> `PIN_WASHER_PUMP_RELAY (D47)`
- `Wiper intermittent` -> `PIN_WIPER_INT_RELAY (D44)`
- `Wiper Normal` -> `PIN_WIPER_LOW_RELAY (D45)`
- `Wiper Fast` -> `PIN_WIPER_HIGH_RELAY (D46)`
- `Ventilation Low` -> `PIN_HEATER_ELEM_1_RELAY (D49)`
- `Ventilation Mid` -> `PIN_HEATER_ELEM_2_RELAY (D50)`
- `Ventilation High` -> `PIN_HEATER_FAN_RELAY (D51)`

## Notes

- `D0/D1` are UART0 serial pins on Arduino Due. Firmware currently protects these from accessory output driving to keep Programming Port serial diagnostics working.
- If you later add persistent EEPROM/flash mapping and the board has stored overrides, defaults in this file will not apply until defaults are restored.

## Input CPU Notes

- User panel controls now come from `Input CPU` command frames.
- Master CPU keeps only physical fail-safe/stalk inputs as direct GPIO inputs.

## ESP32 Master Target (New)

Source: `firmware/master_cpu_esp32/master_cpu_esp32.ino`

Serial links:
- `UART1` (Input CPU): `RX=GPIO16`, `TX=GPIO17`
- `UART2` (Display CPU): `RX=GPIO18`, `TX=GPIO19`

Physical header mapping on `2A54N-ESP32`:
- `GPIO16` -> header pin `27`
- `GPIO17` -> header pin `28`
- `GPIO18` -> header pin `30`
- `GPIO19` -> header pin `31`

Status:
- Software mapping verified.
- Physical interconnect pending `JST Micro 1.25` cables for JC3248 display board UART connectors.

Inputs:
- `Ignition` -> `GPIO32` (pull-up, active low)
- `Brake pedal` -> `GPIO35` (pull-up, active low)
- `Pedal analog` -> `GPIO34` (ADC)

Outputs:
- `Daytime running lights` -> `GPIO2`
- `Front Near` -> `GPIO4`
- `Front High Beam` -> `GPIO5`
- `Indicator Left` -> `GPIO21`
- `Indicator Right` -> `GPIO22`
- `Brake light` -> `GPIO23`
- `Horn` -> `GPIO25`
- `Sprinkler On` -> `GPIO26`
- `Wiper intermittent` -> `GPIO27`
- `Wiper Normal` -> `GPIO14`
- `Wiper Fast` -> `GPIO13`
- `Ventilation Low` -> `GPIO12`
- `Ventilation Mid` -> `GPIO15`
- `Ventilation High` -> `GPIO33`

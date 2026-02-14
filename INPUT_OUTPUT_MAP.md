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
- `PIN_TRIP_RESET_BUTTON = 48`
- `PIN_HEAT_ELEM_1_SW = 49`
- `PIN_HEAT_ELEM_2_SW = 50`
- `PIN_FAN_LOW = 51`
- `PIN_FAN_MID = 33`
- `PIN_FAN_HIGH = 53`
- `PIN_AC_OFF_ALL = 55`
- `PIN_AC_VENT_ONLY = 56`
- `PIN_AC_HEAT_1 = 57`
- `PIN_AC_HEAT_2 = 58`
- `PIN_WIPER_START_USER = 59`
- `PIN_WIPER_STOP_USER = 60`
- `PIN_SPRINKLER_USER = 61`
- `PIN_LIGHTS_NORMAL_USER = 62`
- `PIN_LIGHTS_HIGH_USER = 63`
- `PIN_LIGHTS_OFF_USER = 64`

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
- `PIN_HEATER_ELEM_1_RELAY = 0`
- `PIN_HEATER_ELEM_2_RELAY = 1`
- `PIN_HEATER_FAN_RELAY = 3`
- `PIN_DEMISTER_RELAY = 5`

## Notes

- `D0/D1` are UART0 serial pins on Arduino Due. Firmware currently protects these from accessory output driving to keep Programming Port serial diagnostics working.
- If you later add persistent EEPROM/flash mapping and the board has stored overrides, defaults in this file will not apply until defaults are restored.

## trykknap-panel Discovery (User Confirmed)

User confirmed panel-input-related pins:

- `37, 39, 41, 43, 45, 47, 49, 50, 51, 52, 53`

Current firmware status:

- Configured as **digital inputs**:
  - `D37` (`PIN_WIPER_STALK_LOW`)
  - `D39` (`PIN_WIPER_STALK_INT`)
  - `D41` (`PIN_HORN_BUTTON`)
  - `D49` (`PIN_HEAT_ELEM_1_SW`)
  - `D50` (`PIN_HEAT_ELEM_2_SW`)
  - `D51` (`PIN_FAN_LOW`)
  - `D52` (`PIN_LIGHTS_STALK_PARKING`)
  - `D53` (`PIN_FAN_HIGH`)
- Currently configured as **outputs** (not inputs):
  - `D43` (`PIN_CHARGING_RELAY`)
  - `D45` (`PIN_WIPER_LOW_RELAY`)
  - `D47` (`PIN_WASHER_PUMP_RELAY`)

Naming update applied:

- Relevant input labels now use prefix `trykknap-panel ...` in `PinMap::digitalInputLabel`.

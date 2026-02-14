# Requires PowerShell 3.0+

Clear-Host
Write-Host "======================================================="
Write-Host "Finalizing PinDefinitions.h based on the Ellert IO List"
Write-Host "======================================================="

$content = @"
#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

#include <Arduino.h>

// ====================================================================================
// --- 1. INPUTS (42 Total) ---
// All Digital Inputs will be configured with INPUT_PULLUP (LOW = Active/ON).
// NOTE: 12V inputs MUST use external conditioning (Voltage Divider or Optocoupler)
// ====================================================================================

// --- 1.1. CRITICAL / GEAR / ANALOG INPUTS ---
const int PIN_ACCELERATOR = A0;      // Analog 0-3.3V (Must be 0-3.3V, DO NOT USE DIVIDER TO 12V!)
const int PIN_IGNITION_ON = 22;      // 12V: D22
const int PIN_IGNITION_ACC = 23;     // 12V: D23
const int PIN_IGNITION_START = 24;   // 12V: D24
const int PIN_BRAKE_PEDAL = 25;      // 12V: D25 (CRITICAL)
const int PIN_HANDBRAKE = 26;        // 12V: D26
const int PIN_GEAR_PARK = 27;        // 3.3V: D27
const int PIN_GEAR_REVERSE = 28;     // 3.3V: D28
const int PIN_GEAR_NEUTRAL = 29;     // 3.3V: D29
const int PIN_GEAR_DRIVE = 30;       // 3.3V: D30

// --- 1.2. STALKS & DEDICATED SWITCHES (12V) ---
const int PIN_IND_STALK_LEFT = 31;   // 12V: D31
const int PIN_IND_STALK_RIGHT = 32;  // 12V: D32
const int PIN_LIGHTS_STALK_PARKING = 33; // 12V: D33
const int PIN_LIGHTS_STALK_HEAD = 34; // 12V: D34
const int PIN_HIGH_BEAM_STALK = 35;  // 12V: D35 (Flash-to-pass)
const int PIN_HAZARD_SWITCH = 36;    // 12V: D36
const int PIN_WIPER_STALK_LOW = 37;  // 12V: D37
const int PIN_WIPER_STALK_HIGH = 38; // 12V: D38
const int PIN_WIPER_STALK_INT = 39;  // 12V: D39
const int PIN_WASHER_SWITCH = 40;    // 12V: D40
const int PIN_HORN_BUTTON = 41;      // 12V: D41
const int PIN_DEMISTER_SWITCH = 42;  // 12V: D42

// --- 1.3. USER PANEL SWITCHES (3.3V) ---
// NOTE: Due pins D55-D64 are aliases for Analog Pins A1-A10
const int PIN_TRIP_RESET_BUTTON = 48; // D48
const int PIN_HEAT_ELEM_1 = 49;       // D49
const int PIN_HEAT_ELEM_2 = 50;       // D50
const int PIN_FAN_LOW = 51;           // D51
const int PIN_FAN_MID = 52;           // D52
const int PIN_FAN_HIGH = 53;          // D53
const int PIN_AC_OFF_ALL = 55;        // A1 (D55)
const int PIN_AC_VENT_ONLY = 56;      // A2 (D56)
const int PIN_AC_HEAT_1 = 57;         // A3 (D57)
const int PIN_AC_HEAT_2 = 58;         // A4 (D58)
const int PIN_WIPER_START_USER = 59;  // A5 (D59)
const int PIN_WIPER_STOP_USER = 60;   // A6 (D60)
const int PIN_SPRINKLER_USER = 61;    // A7 (D61)
const int PIN_LIGHTS_NORMAL_USER = 62; // A8 (D62)
const int PIN_LIGHTS_HIGH_USER = 63;  // A9 (D63)
const int PIN_LIGHTS_OFF_USER = 64;   // A10 (D64)


// ====================================================================================
// --- 2. OUTPUTS (23 Total) ---
// NOTE: These pins control the 3.3V control-side of your Solid State Relays (SSRs).
// Pins D9, D10, D20, D21, D3, D5, DAC0 are left free for LCD/Touch Interfaces.
// ====================================================================================

// --- 2.1. CRITICAL / MOTOR CONTROL ---
const int PIN_MOTOR_PWM_OUT = DAC0;         // DAC0 (REMOVED: Motor Control is external in this phase)
const int PIN_MAIN_CONTACTOR = 2;           // D2 (CRITICAL!)
const int PIN_BRAKE_LIGHTS_RELAY = 4;       // D4 (CRITICAL!)

// --- 2.2. LIGHTING & AUXILIARY OUTPUTS ---
const int PIN_HEADLIGHTS_RELAY = 6;         // D6
const int PIN_HIGH_BEAMS_RELAY = 7;         // D7
const int PIN_PARKING_LIGHTS_RELAY = 8;     // D8 (Combined Front L/R)
const int PIN_IND_LEFT_RELAY = 14;          // D14 (TX3)
const int PIN_IND_RIGHT_RELAY = 15;         // D15 (RX3)
const int PIN_HORN_RELAY = 16;              // D16 (TX2)
const int PIN_REVERSE_LIGHT_RELAY = 17;     // D17 (RX2)
const int PIN_P_BRAKE_INDICATOR = 18;       // D18 (TX1)
const int PIN_INTERIOR_LIGHT_RELAY = 19;    // D19 (RX1)
const int PIN_CHARGING_RELAY = 43;          // D43 (Using free pin)

// --- 2.3. WIPER & WASHER OUTPUTS ---
const int PIN_WIPER_INT_RELAY = 44;         // D44
const int PIN_WIPER_LOW_RELAY = 45;         // D45
const int PIN_WIPER_HIGH_RELAY = 46;        // D46
const int PIN_WASHER_PUMP_RELAY = 47;       // D47

// --- 2.4. HEATING & DEFROST OUTPUTS ---
const int PIN_HEATER_ELEM_1_RELAY = 5;      // D5
const int PIN_HEATER_ELEM_2_RELAY = 12;     // D12 (MISO)
const int PIN_HEATER_FAN_RELAY = 13;        // D13 (SCK)
const int PIN_DEMISTER_RELAY = 11;          // D11 (MOSI)

#endif // PIN_DEFINITIONS_H
"@
$content | Out-File "PinDefinitions.h" -Encoding UTF8
Write-Host "Content written to PinDefinitions.h with finalized pin assignments."

# The terminal window will stay open after the script finishes execution.
Read-Host -Prompt "Press Enter to exit"
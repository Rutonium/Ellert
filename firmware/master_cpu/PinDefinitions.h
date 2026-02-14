#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

#include <Arduino.h>

// ====================================================================================
// --- 1. INPUTS (42 Total) ---
// All Digital Inputs will be configured with INPUT_PULLUP (LOW = Active/ON).
// NOTE: 12V inputs MUST use external conditioning (Voltage Divider or Optocoupler)
// ====================================================================================

// --- 1.1. CRITICAL / GEAR / ANALOG INPUTS ---
const int PIN_ACCELERATOR = A0;      
const int PIN_IGNITION_ON = 22;      
const int PIN_IGNITION_ACC = 23;     
const int PIN_IGNITION_START = 24;   
const int PIN_BRAKE_PEDAL = 25;      
const int PIN_HANDBRAKE = 26;        
const int PIN_GEAR_PARK = 27;        
const int PIN_GEAR_REVERSE = 28;     
const int PIN_GEAR_NEUTRAL = 29;     
const int PIN_GEAR_DRIVE = 30;       

// --- 1.2. STALKS & DEDICATED SWITCHES (12V) ---
const int PIN_IND_STALK_LEFT = 31;   
const int PIN_IND_STALK_RIGHT = 32;  
const int PIN_LIGHTS_STALK_PARKING = 52; // Position light toggle button (confirmed)
const int PIN_LIGHTS_STALK_HEAD = 34; 
const int PIN_HIGH_BEAM_STALK = 35;  
const int PIN_HAZARD_SWITCH = 36;    
const int PIN_WIPER_STALK_LOW = 37;  
const int PIN_WIPER_STALK_HIGH = 38; 
const int PIN_WIPER_STALK_INT = 39;  
const int PIN_WASHER_SWITCH = 40;    
const int PIN_HORN_BUTTON = 41;      
const int PIN_DEMISTER_SWITCH = 42;  

// --- 1.3. USER PANEL SWITCHES (3.3V) ---
const int PIN_TRIP_RESET_BUTTON = 48; 
const int PIN_HEAT_ELEM_1_SW = 49;       // D49 (SW to differentiate from RELAY)
const int PIN_HEAT_ELEM_2_SW = 50;       // D50 
const int PIN_FAN_LOW = 51;           
const int PIN_FAN_MID = 33;           // moved to keep unique input pin assignment
const int PIN_FAN_HIGH = 53;          
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
// SSR Board Assignments: SSR1(D2,D4,D6,D7), SSR2(D8,D14,D15,D16), SSR3(D17,D18,D19,D43), SSR4(D44,D45,D46,D47)
// ====================================================================================

// --- 2.1. CRITICAL / MOTOR CONTROL ---
const int PIN_MOTOR_PWM_OUT = DAC0;         // DAC0 (Interface only, not managed in this phase)
const int PIN_MAIN_CONTACTOR = 2;           // D2 (SSR1-R1 - CRITICAL!)

// --- 2.2. LIGHTING & AUXILIARY OUTPUTS ---
const int PIN_BRAKE_LIGHTS_RELAY = 4;       // D4 (SSR1-R2)
const int PIN_HEADLIGHTS_RELAY = 6;         // D6 (SSR1-R3)
const int PIN_HIGH_BEAMS_RELAY = 7;         // D7 (SSR1-R4)
const int PIN_PARKING_LIGHTS_RELAY = 8;     // D8 (SSR2-R1)
const int PIN_IND_LEFT_RELAY = 14;          // D14 (SSR2-R2)
const int PIN_IND_RIGHT_RELAY = 15;         // D15 (SSR2-R3)
const int PIN_HORN_RELAY = 16;              // D16 (SSR2-R4)
const int PIN_REVERSE_LIGHT_RELAY = 17;     // D17 (SSR3-R1)
const int PIN_P_BRAKE_INDICATOR = 18;       // D18 (SSR3-R2)
const int PIN_INTERIOR_LIGHT_RELAY = 19;    // D19 (SSR3-R3)
const int PIN_CHARGING_RELAY = 43;          // D43 (SSR3-R4)

// --- 2.3. WIPER & WASHER OUTPUTS ---
const int PIN_WIPER_INT_RELAY = 44;         // D44 (SSR4-R1)
const int PIN_WIPER_LOW_RELAY = 45;         // D45 (SSR4-R2)
const int PIN_WIPER_HIGH_RELAY = 46;        // D46 (SSR4-R3)
const int PIN_WASHER_PUMP_RELAY = 47;       // D47 (SSR4-R4)

// --- 2.4. HEATING & DEFROST OUTPUTS (Need a 5th SSR board for these) ---
// NOTE: These defaults avoid conflicts with input pins, but they use Serial/Touch pins.
// If you use Serial or the touch interface, move these to free pins in your wiring map.
const int PIN_HEATER_ELEM_1_RELAY = 0;      // D0 (Serial RX0)
const int PIN_HEATER_ELEM_2_RELAY = 1;      // D1 (Serial TX0)
const int PIN_HEATER_FAN_RELAY = 3;         // D3 (Touch INT reserved)
const int PIN_DEMISTER_RELAY = 5;           // D5 (Touch RESET reserved)

// --- 2.5. LCD/TOUCH INTERFACE PINS (Reserved) ---
// D9, D10, D11, D12, D13, D20, D21, D3, D5 are RESERVED for the RA8875 Shield.

#endif // PIN_DEFINITIONS_H

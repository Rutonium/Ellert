#ifndef PIN_CONSTRAINTS_H
#define PIN_CONSTRAINTS_H

#include <Arduino.h>

// Pin constraint lists used for validation and UI warnings.
// Leave lists empty to disable validation for that category.

// Pins that should never be assigned (e.g., SPI, I2C, Touch, Serial).
// Fill these later when you know your reserved pins.
constexpr int RESERVED_PINS[] = {
    // Example: 9, 10, 11, 12, 13, 20, 21, 3, 5
};
constexpr int RESERVED_PINS_COUNT = sizeof(RESERVED_PINS) / sizeof(RESERVED_PINS[0]);

// Analog-capable pins (for AnalogInputId entries).
// Default list for Arduino Due analog inputs.
constexpr int ALLOWED_ANALOG_PINS[] = {
    A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11
};
constexpr int ALLOWED_ANALOG_PINS_COUNT = sizeof(ALLOWED_ANALOG_PINS) / sizeof(ALLOWED_ANALOG_PINS[0]);

// PWM-capable pins (for outputs that require PWM).
// Leave empty until you confirm the pins you want to allow.
constexpr int ALLOWED_PWM_PINS[] = {
    // Example: 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13
};
constexpr int ALLOWED_PWM_PINS_COUNT = sizeof(ALLOWED_PWM_PINS) / sizeof(ALLOWED_PWM_PINS[0]);

#endif // PIN_CONSTRAINTS_H

#include "MotorControl.h"
#include "PinMap.h"
#include "InputFilter.h"

void MotorControl::initialize() {
    // --- OUTPUTS ---
    pinMode(PinMap::output(PinMap::OutputId::MainContactor), OUTPUT);
    digitalWrite(PinMap::output(PinMap::OutputId::MainContactor), LOW); // Start Safe: Main Contactor OFF

    // --- INPUTS (Motor/Gear) ---
    pinMode(PinMap::digitalInput(PinMap::DigitalInputId::IgnitionOn), INPUT_PULLUP);
    pinMode(PinMap::digitalInput(PinMap::DigitalInputId::IgnitionStart), INPUT_PULLUP);
    pinMode(PinMap::digitalInput(PinMap::DigitalInputId::BrakePedal), INPUT_PULLUP);
    pinMode(PinMap::digitalInput(PinMap::DigitalInputId::Handbrake), INPUT_PULLUP);
    pinMode(PinMap::digitalInput(PinMap::DigitalInputId::GearPark), INPUT_PULLUP);
    pinMode(PinMap::digitalInput(PinMap::DigitalInputId::GearReverse), INPUT_PULLUP);
    pinMode(PinMap::digitalInput(PinMap::DigitalInputId::GearNeutral), INPUT_PULLUP);
    pinMode(PinMap::digitalInput(PinMap::DigitalInputId::GearDrive), INPUT_PULLUP);
    pinMode(PinMap::analogInput(PinMap::AnalogInputId::Accelerator), INPUT); // Analog input
}

void MotorControl::update() {
    // LOW = Active/ON for all inputs (due to INPUT_PULLUP)
    bool key_on = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::IgnitionOn)) == LOW);
    bool brake_on = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::BrakePedal)) == LOW);
    bool handbrake_on = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::Handbrake)) == LOW);
    
    // 1. Main Contactor Logic (Master Power)
    // Contactor is ON only if Key is ON (RUN) and the Handbrake is NOT engaged.
    if (key_on && !handbrake_on) {
        digitalWrite(PinMap::output(PinMap::OutputId::MainContactor), HIGH); // Engage main power bus
    } else {
        digitalWrite(PinMap::output(PinMap::OutputId::MainContactor), LOW);  // Disengage main power bus (CRITICAL SAFETY)
    }

    // 2. Motor Kill Signal (Future Implementation)
    // The "kill signal" logic for the external motor controller would go here.
    // Assuming the motor controller has an Active LOW kill signal input:
    // digitalWrite(PIN_MOTOR_KILL_SIGNAL, brake_on ? LOW : HIGH); 
    // This is currently disabled since the motor control interface is separate.
    
    // Optional Debug: Print Critical States
    // static unsigned long lastDebugTime = 0;
    // if (millis() - lastDebugTime > 500) {
    //     Serial.print("M: Contactor="); Serial.print(digitalRead(PIN_MAIN_CONTACTOR));
    //     Serial.print(" Key="); Serial.print(key_on);
    //     Serial.print(" Brake="); Serial.print(brake_on);
    //     Serial.print(" Handbrake="); Serial.println(handbrake_on);
    //     lastDebugTime = millis();
    // }
}

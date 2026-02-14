#include "AccessoryControl.h"
#include "PinMap.h"
#include "InputFilter.h"

namespace {
bool isProtectedSerialPin(int pin) {
    return pin == 0 || pin == 1;
}
}

void AccessoryControl::initialize() {
    // --- OUTPUTS (Accessory Relays) ---
    for (int i = 0; i < PinMap::kOutputCount; ++i) {
        int pin = PinMap::outputByIndex(i);
        if (isProtectedSerialPin(pin)) continue;
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
    
    // --- INPUTS (Stalks/User Panel) ---
    for (int i = 0; i < PinMap::kDigitalCount; ++i) {
        pinMode(PinMap::digitalInputByIndex(i), INPUT_PULLUP);
    }
}

void AccessoryControl::handleLights() {
    // Read stable states
    bool key_on = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::IgnitionOn)) == LOW);
    bool brake_on = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::BrakePedal)) == LOW);
    bool parking_lights_on = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::LightsStalkParking)) == LOW);
    bool headlights_on = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::LightsStalkHead)) == LOW);
    bool high_beam_on = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::HighBeamStalk)) == LOW);
    bool reverse_gear_on = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::GearReverse)) == LOW);
    bool handbrake_on = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::Handbrake)) == LOW);
    
    // --- 1. Basic Lights (NON-FSM) ---
    digitalWrite(PinMap::output(PinMap::OutputId::BrakeLightsRelay), brake_on ? HIGH : LOW);
    digitalWrite(PinMap::output(PinMap::OutputId::PBrakeIndicator), handbrake_on ? HIGH : LOW);
    digitalWrite(PinMap::output(PinMap::OutputId::ReverseLightRelay), reverse_gear_on ? HIGH : LOW);
    
    // --- Headlights/Parking Lights ---
    if (key_on) {
        digitalWrite(PinMap::output(PinMap::OutputId::ParkingLightsRelay), parking_lights_on ? HIGH : LOW);
        bool normal_lights_on = (parking_lights_on || headlights_on);
        digitalWrite(PinMap::output(PinMap::OutputId::HeadlightsRelay), normal_lights_on ? HIGH : LOW);
        digitalWrite(PinMap::output(PinMap::OutputId::HighBeamsRelay), (digitalRead(PinMap::output(PinMap::OutputId::HeadlightsRelay)) == HIGH && high_beam_on) ? HIGH : LOW);
    } else {
        digitalWrite(PinMap::output(PinMap::OutputId::ParkingLightsRelay), LOW);
        digitalWrite(PinMap::output(PinMap::OutputId::HeadlightsRelay), LOW);
        digitalWrite(PinMap::output(PinMap::OutputId::HighBeamsRelay), LOW);
    }

    // --- 2. Indicator Flasher (FSM - Confirmed Working) ---
    static unsigned long lastFlashTime = 0;
    static bool flashState = LOW; 
    const unsigned long FLASH_INTERVAL = 500; 

    bool left_turn = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::IndStalkLeft)) == LOW);
    bool right_turn = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::IndStalkRight)) == LOW);
    bool hazards_on = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::HazardSwitch)) == LOW);
        
    if (left_turn || right_turn || hazards_on) {
        if (millis() - lastFlashTime >= FLASH_INTERVAL) {
            flashState = !flashState; 
            lastFlashTime = millis();
        }
        
        if (left_turn || hazards_on) {
            digitalWrite(PinMap::output(PinMap::OutputId::IndLeftRelay), flashState); 
        } else {
            digitalWrite(PinMap::output(PinMap::OutputId::IndLeftRelay), LOW); 
        }
        
        if (right_turn || hazards_on) {
            digitalWrite(PinMap::output(PinMap::OutputId::IndRightRelay), flashState); 
        } else {
            digitalWrite(PinMap::output(PinMap::OutputId::IndRightRelay), LOW); 
        }
    } else {
        flashState = LOW; 
        digitalWrite(PinMap::output(PinMap::OutputId::IndLeftRelay), LOW);
        digitalWrite(PinMap::output(PinMap::OutputId::IndRightRelay), LOW);
    }
}

void AccessoryControl::handleWipers() {
    // Read Wiper Stalk/Switch Inputs
    bool wiper_low = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::WiperStalkLow)) == LOW); 
    bool wiper_high = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::WiperStalkHigh)) == LOW); 
    bool wiper_int = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::WiperStalkInt)) == LOW); 
    bool washer_on = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::WasherSwitch)) == LOW); 

    // --- 1. Set Washer Pump Relay ---
    digitalWrite(PinMap::output(PinMap::OutputId::WasherPumpRelay), washer_on ? HIGH : LOW); 

    // --- 2. Continuous Wiper Logic (Priority) ---
    if (wiper_high) {
        digitalWrite(PinMap::output(PinMap::OutputId::WiperHighRelay), HIGH); 
        digitalWrite(PinMap::output(PinMap::OutputId::WiperLowRelay), LOW);  
        digitalWrite(PinMap::output(PinMap::OutputId::WiperIntRelay), LOW);  
    } 
    else if (wiper_low) {
        digitalWrite(PinMap::output(PinMap::OutputId::WiperLowRelay), HIGH); 
        digitalWrite(PinMap::output(PinMap::OutputId::WiperHighRelay), LOW);
        digitalWrite(PinMap::output(PinMap::OutputId::WiperIntRelay), LOW);
    } 
    
    // --- 3. Intermittent Wiper FSM ---
    else if (wiper_int) {
        const unsigned long WIPE_TIME = 1500; // 1.5 seconds ON
        const unsigned long PAUSE_TIME = 5000; // 5.0 seconds OFF (Pause)
        
        static unsigned long lastWipeToggle = 0;
        static bool isWiping = false; 

        if (isWiping) {
            if (millis() - lastWipeToggle >= WIPE_TIME) {
                isWiping = false; 
                lastWipeToggle = millis();
            }
        } else {
            if (millis() - lastWipeToggle >= PAUSE_TIME) {
                isWiping = true; 
                lastWipeToggle = millis();
            }
        }
        
        digitalWrite(PinMap::output(PinMap::OutputId::WiperLowRelay), isWiping ? HIGH : LOW);
        digitalWrite(PinMap::output(PinMap::OutputId::WiperHighRelay), LOW);
        digitalWrite(PinMap::output(PinMap::OutputId::WiperIntRelay), HIGH); 
    }
    
    // --- 4. Wipers OFF ---
    else {
        digitalWrite(PinMap::output(PinMap::OutputId::WiperLowRelay), LOW);
        digitalWrite(PinMap::output(PinMap::OutputId::WiperHighRelay), LOW);
        digitalWrite(PinMap::output(PinMap::OutputId::WiperIntRelay), LOW);
    }
}

void AccessoryControl::handleHeating() {
    // Heating logic is driven by user panel switches (D50-D53, A3-A4)

    // Read Input States (LOW = Active)
    bool heater1_sw = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::HeatElem1Sw)) == LOW);
    bool heater2_sw = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::HeatElem2Sw)) == LOW);
    bool fan_low = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::FanLow)) == LOW);
    bool fan_mid = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::FanMid)) == LOW);
    bool fan_high = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::FanHigh)) == LOW);
    bool demister_sw = (InputFilter::getDebouncedState(PinMap::digitalInput(PinMap::DigitalInputId::DemisterSwitch)) == LOW);

    // --- 1. Heater Elements ---
    // Elements are typically ON if the corresponding switch is ON
    const int heater1Pin = PinMap::output(PinMap::OutputId::HeaterElem1Relay);
    const int heater2Pin = PinMap::output(PinMap::OutputId::HeaterElem2Relay);
    if (!isProtectedSerialPin(heater1Pin)) digitalWrite(heater1Pin, heater1_sw ? HIGH : LOW);
    if (!isProtectedSerialPin(heater2Pin)) digitalWrite(heater2Pin, heater2_sw ? HIGH : LOW);
    
    // --- 2. Demister (Heated Rear Window) ---
    // Demister switch often requires a timer, but for simplicity:
    digitalWrite(PinMap::output(PinMap::OutputId::DemisterRelay), demister_sw ? HIGH : LOW);

    // --- 3. Fan Control ---
    // Assuming PIN_HEATER_FAN_RELAY controls the single fan, and the fan speed
    // is controlled by a separate power resistor or a multi-speed motor.
    // We will use a simple priority: High > Mid > Low > Off.
    if (fan_high) {
        digitalWrite(PinMap::output(PinMap::OutputId::HeaterFanRelay), HIGH); // Max speed (Needs power staging if fan has multiple inputs)
    } else if (fan_mid) {
        digitalWrite(PinMap::output(PinMap::OutputId::HeaterFanRelay), HIGH); // Medium speed
    } else if (fan_low) {
        digitalWrite(PinMap::output(PinMap::OutputId::HeaterFanRelay), HIGH); // Low speed
    } else {
        digitalWrite(PinMap::output(PinMap::OutputId::HeaterFanRelay), LOW);
    }
}

void AccessoryControl::update() {
    handleLights();
    handleWipers();
    handleHeating();
}

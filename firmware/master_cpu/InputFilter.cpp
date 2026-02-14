#include "InputFilter.h"
#include "PinMap.h"

// Static variables for debounce state and time.
int lastDebounceState[PinMap::kDigitalCount]; 
unsigned long lastDebounceTime[PinMap::kDigitalCount] = {0}; 
// ---------------------------------------------------------------------------------

void InputFilter::initialize() {
    for (int i = 0; i < PinMap::kDigitalCount; ++i) {
        lastDebounceState[i] = HIGH; 
    }
}

void InputFilter::update() {
    const unsigned long DEBOUNCE_DELAY = 50; // 50 milliseconds
    
    for (int i = 0; i < PinMap::kDigitalCount; ++i) {
        int pin = PinMap::digitalInputByIndex(i);
        int reading = digitalRead(pin); 

        if (reading != lastDebounceState[i]) {
            if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
                lastDebounceState[i] = reading;
                // DO NOT update lastDebounceTime here. It is used to track the last transition time.
            }
        } else {
             // If the state is the same, update the debounce timer to NOW for robustness.
             lastDebounceTime[i] = millis();
        }
    }
}

int InputFilter::getDebouncedState(int pin) { 
    for (int i = 0; i < PinMap::kDigitalCount; ++i) {
        if (PinMap::digitalInputByIndex(i) == pin) {
            return lastDebounceState[i];
        }
    }
    return HIGH; 
}

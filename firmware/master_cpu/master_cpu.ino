#include "InputFilter.h"
#include "MotorControl.h"
#include "AccessoryControl.h"
#include "DisplayManager.h" // NEW
#include "SerialDiagnostics.h"
#include "RemoteInterfaces.h"
#include "PinMap.h"
#include "PinRules.h"

MotorControl motorController;
AccessoryControl accessoryController;
DisplayManager displayManager; // NEW

void setup() {
    // Start the Serial Monitor/Debugger
    Serial.begin(115200); 
    
    PinRules::load();
    PinMap::load();
    if (!PinMap::validateNoConflicts()) {
        Serial.println("Pin map conflicts detected. Fix pin map and reboot.");
        while (1) { }
    }
    
    // Initialize all pin modes and starting states
    motorController.initialize();
    accessoryController.initialize();
    InputFilter::initialize(); 
    displayManager.initialize(); // NEW
    SerialDiagnostics::initialize();
    RemoteInterfaces::initialize();
}

void loop() {
    // 1. Process all input states (debouncing, voltage checks, etc.)
    InputFilter::update();
    
    if (PinMap::consumeDirty()) {
        motorController.initialize();
        accessoryController.initialize();
        InputFilter::initialize();
    }

    // 2. Run Critical Motor Control Logic (Safety FIRST)
    motorController.update();

    // 3. Run Standard Accessory Control Logic
    accessoryController.update();
    
    // 4. Update the Screen Display
    displayManager.update(); // NEW

    // 5. Publish full board diagnostics to serial for PC mock display
    SerialDiagnostics::update();

    // 6. Handle optional Input/Display CPU links
    RemoteInterfaces::update();
}

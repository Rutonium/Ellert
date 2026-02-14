# Requires PowerShell 3.0+

Clear-Host
Write-Host "======================================================="
Write-Host "Implementing DisplayManager for Live I/O Monitoring"
Write-Host "======================================================="

$PROJ_NAME = "Ellert"

# --- 1. Create DisplayManager.h ---
Write-Host "1. Creating DisplayManager.h..."
$content = @"
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_RA8875.h>

class DisplayManager {
public:
    void initialize();
    void update();
private:
    void drawStatusGrid(int x, int y, const char* label, int pin, bool isInput);
};
#endif // DISPLAY_MANAGER_H
"@
$content | Out-File "DisplayManager.h" -Encoding UTF8


# --- 2. Create DisplayManager.cpp ---
Write-Host "2. Creating DisplayManager.cpp..."
$content = @"
#include "DisplayManager.h"
#include "PinDefinitions.h"
#include "InputFilter.h"

// Define the pins used by the RA8875 Shield. These were reserved in PinDefinitions.h.
#define RA8875_CS 10  // D10
#define RA8875_RESET 9 // D9

// Create the display object (SPI interface pins are fixed by the Due's SPI bus)
Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);

void DisplayManager::initialize() {
    Serial.println("DisplayManager: Initializing RA8875...");
    if (!tft.begin(RA8875_480x272)) { // Set your specific resolution here if different
        Serial.println("RA8875 Not Found or Initialization Failed!");
        while (1);
    }
    
    // Set colors, orientation, etc.
    tft.displayOn(true);
    tft.showCursor(false);
    tft.fillScreen(RA8875_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(RA8875_WHITE);

    // Initial labels for the screen
    tft.cursorTo(5, 5);
    tft.write("ELLERT UPGRADE - LIVE I/O MONITOR");
}

void DisplayManager::drawStatusGrid(int x, int y, const char* label, int pin, bool isInput) {
    // Read the current pin state (debounced for input, raw for output)
    int state = isInput ? InputFilter::getDebouncedState(pin) : digitalRead(pin);
    
    // Determine the color: GREEN for ON/LOW, RED for OFF/HIGH (for input, LOW is usually 'ON')
    uint16_t color = RA8875_GREEN; 
    
    if (isInput) {
        if (state == HIGH) color = RA8875_RED; // Input HIGH (OFF state) is RED
    } else {
        if (state == LOW) color = RA8875_RED; // Output LOW (OFF state) is RED
    }

    // Clear the drawing area for status update
    tft.fillRect(x, y, 100, 15, RA8875_BLACK); 
    
    // Draw the Label
    tft.cursorTo(x, y);
    tft.setTextColor(RA8875_WHITE);
    tft.write(label);

    // Draw the Status
    tft.cursorTo(x + 70, y);
    tft.setTextColor(color);
    tft.write(state == HIGH ? "HIGH" : "LOW ");
}

void DisplayManager::update() {
    // We only update the screen every 100ms to keep the performance high
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 100) return;
    lastUpdate = millis();

    int row = 20;

    // --- CRITICAL INPUTS ---
    drawStatusGrid(5, row, "Ignition ON (D22):", PIN_IGNITION_ON, true); row += 10;
    drawStatusGrid(5, row, "Brake Pedal (D25):", PIN_BRAKE_PEDAL, true); row += 10;
    drawStatusGrid(5, row, "Handbrake (D26):", PIN_HANDBRAKE, true); row += 10;
    drawStatusGrid(5, row, "Gear D (D30):", PIN_GEAR_DRIVE, true); row += 20;

    // --- CRITICAL OUTPUTS ---
    drawStatusGrid(5, row, "MAIN CONTR (D2):", PIN_MAIN_CONTACTOR, false); row += 10;
    drawStatusGrid(5, row, "BRAKE L. (D4):", PIN_BRAKE_LIGHTS_RELAY, false); row += 20;

    // --- LIGHTING OUTPUTS (Example using Indicator Pins) ---
    drawStatusGrid(5, row, "IND. LEFT (D14):", PIN_IND_LEFT_RELAY, false); row += 10;
    drawStatusGrid(5, row, "IND. RIGHT (D15):", PIN_IND_RIGHT_RELAY, false); row += 10;
    
    // --- Add more I/O to this function as needed, up to the screen limit ---

}
"@
$content | Out-File "DisplayManager.cpp" -Encoding UTF8
Write-Host "DisplayManager.cpp created."


# --- 3. Update Ellert.ino to use the new DisplayManager ---
Write-Host "3. Updating Ellert.ino to integrate DisplayManager..."
$content = @"
#include "PinDefinitions.h"
#include "InputFilter.h"
#include "MotorControl.h"
#include "AccessoryControl.h"
#include "DisplayManager.h" // NEW

MotorControl motorController;
AccessoryControl accessoryController;
DisplayManager displayManager; // NEW

void setup() {
    // Start the Serial Monitor/Debugger
    Serial.begin(115200); 
    
    // Initialize all pin modes and starting states
    motorController.initialize();
    accessoryController.initialize();
    InputFilter::initialize(); 
    displayManager.initialize(); // NEW
}

void loop() {
    // 1. Process all input states (debouncing, voltage checks, etc.)
    InputFilter::update();
    
    // 2. Run Critical Motor Control Logic (Safety FIRST)
    motorController.update();

    // 3. Run Standard Accessory Control Logic
    accessoryController.update();
    
    // 4. Update the Screen Display
    displayManager.update(); // NEW
}
"@
$content | Out-File "$PROJ_NAME.ino" -Encoding UTF8
Write-Host "Ellert.ino updated."

Write-Host "======================================================="
Write-Host "Screen Display Module is now implemented!"
Write-Host "Next Step: Compile, Upload, and Connect the Shield."
Write-Host "======================================================="

Read-Host -Prompt "Press Enter to exit"
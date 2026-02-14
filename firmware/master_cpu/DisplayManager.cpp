#include "DisplayManager.h"
#include "PinMap.h"
#include "InputFilter.h"
#include "TouchManager.h"
#include "PinRules.h"
#include <cstring>

// Define the pins used by the RA8875 Shield.
#define RA8875_CS 10  // D10
#define RA8875_RESET 9 // D9

// Create the display object (SPI interface pins are fixed by the Due's SPI bus)
Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);
TouchManager touch;

constexpr int SCREEN_W = 480;
constexpr int SCREEN_H = 272;
constexpr int HEADER_H = 22;
constexpr int ROW_H = 18;

void DisplayManager::initialize() {
    Serial.println("DisplayManager: Initializing RA8875...");
    if (!tft.begin(RA8875_480x272)) {
        Serial.println("RA8875 not found; running headless serial mode.");
        displayAvailable = false;
        return;
    }
    displayAvailable = true;

    // Set colors, orientation, etc.
    tft.displayOn(true);
    tft.fillScreen(RA8875_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(RA8875_WHITE);

    touch.begin();
    screenDirty = true;
    render();
}

void DisplayManager::drawStatusGrid(int x, int y, const char* label, int pin, bool isInput) {
    // Read the current pin state (debounced for input, raw for output)
    int state = isInput ? InputFilter::getDebouncedState(pin) : digitalRead(pin);

    // Determine the color: GREEN for ON/LOW, RED for OFF/HIGH
    uint16_t color = RA8875_GREEN;

    if (isInput) {
        if (state == HIGH) color = RA8875_RED; // Input HIGH (OFF state) is RED
    } else {
        if (state == LOW) color = RA8875_RED; // Output LOW (OFF state) is RED
    }

    // Clear the drawing area for status update
    tft.fillRect(x, y, 100, 15, RA8875_BLACK);

    // Draw the Label
    tft.setCursor(x, y);
    tft.setTextColor(RA8875_WHITE);
    tft.print(label);
    tft.print(" (D");
    tft.print(pin);
    tft.print("):");

    // Draw the Status
    tft.setCursor(x + 70, y);
    tft.setTextColor(color);
    tft.print(state == HIGH ? "HIGH" : "LOW ");
}

void DisplayManager::update() {
    if (!displayAvailable) return;
    handleTouch();

    // Throttle full re-render to keep performance high
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 80 && !screenDirty) return;
    lastUpdate = millis();
    render();
}

void DisplayManager::drawButton(int x, int y, int w, int h, const char* label, bool filled) {
    if (filled) {
        tft.fillRect(x, y, w, h, RA8875_BLUE);
    } else {
        tft.drawRect(x, y, w, h, RA8875_WHITE);
    }
    tft.setCursor(x + 4, y + 5);
    tft.setTextColor(RA8875_WHITE);
    tft.print(label);
}

void DisplayManager::drawHeader(const char* title) {
    tft.fillRect(0, 0, SCREEN_W, HEADER_H, RA8875_BLACK);
    tft.setCursor(5, 5);
    tft.setTextColor(RA8875_WHITE);
    tft.print(title);
}

void DisplayManager::logStatus(const char* msg, bool isError) {
    strncpy(logMsgs[logHead], msg, sizeof(logMsgs[logHead]) - 1);
    logMsgs[logHead][sizeof(logMsgs[logHead]) - 1] = '\0';
    logHead = (logHead + 1) % 10;
    if (logCount < 10) logCount++;
    statusIsError = isError;
}

bool DisplayManager::hitBox(int x, int y, int w, int h, int tx, int ty) const {
    return (tx >= x && tx <= (x + w) && ty >= y && ty <= (y + h));
}

void DisplayManager::handleTouch() {
    int tx = 0;
    int ty = 0;
    if (!touch.readTouch(tx, ty)) {
        touchActive = false;
        return;
    }
    touchActive = true;
    touchX = tx;
    touchY = ty;
    if (currentScreen == Screen::TouchTest) {
        screenDirty = true;
    }

    // Basic debounce
    if (millis() - lastTouchMs < 200) return;
    lastTouchMs = millis();

    if (currentScreen == Screen::LiveMonitor) {
        if (hitBox(400, 0, 80, HEADER_H, tx, ty)) {
            currentScreen = Screen::ServiceMenu;
            screenDirty = true;
        }
        return;
    }

    if (kbVisible) {
        // Keyboard is non-blocking: only handle touches inside keyboard area.
        const int kbY = 150;
        if (ty >= kbY) {
            // Keyboard touch handling
            // Rows: 0-2 letters, row 3 control
            const char* row1 = "1234567890";
            const char* row2 = "QWERTYUIOP";
            const char* row3 = "ASDFGHJKL";
            const char* row4 = "ZXCVBNM";
            const int keyW = 40;
            const int keyH = 28;
            int row = (ty - kbY) / keyH;
            int col = tx / keyW;
            if (row == 0 && col < 10) {
                if (kbLen < 12) kbBuffer[kbLen++] = row1[col];
            } else if (row == 1 && col < 10) {
                if (kbLen < 12) kbBuffer[kbLen++] = row2[col];
            } else if (row == 2 && col < 9) {
                if (kbLen < 12) kbBuffer[kbLen++] = row3[col];
            } else if (row == 3) {
                if (col < 7) {
                    if (kbLen < 12) kbBuffer[kbLen++] = row4[col];
                } else if (col == 7) {
                    if (kbLen < 12) kbBuffer[kbLen++] = '_';
                } else if (col == 8) {
                    if (kbLen > 0) kbLen--;
                } else if (col == 9) {
                    // OK
                    kbBuffer[kbLen] = '\0';
                    bool ok = false;
                    if (editType == EditType::Reserved) {
                        ok = PinRules::setReservedLabelAt(ruleIndex, kbBuffer);
                    } else if (editType == EditType::Pwm) {
                        ok = PinRules::setPwmLabelAt(ruleIndex, kbBuffer);
                    }
                    if (ok) {
                        PinRules::save();
                        logStatus("Label saved", false);
                    } else {
                        logStatus("Label save failed", true);
                    }
                    kbVisible = false;
                }
            }
            kbBuffer[kbLen] = '\0';
            screenDirty = true;
            return;
        }
        // Do not block other touches outside keyboard area
    }

    if (currentScreen == Screen::ServiceMenu) {
        if (hitBox(0, 0, 60, HEADER_H, tx, ty)) {
            currentScreen = Screen::LiveMonitor;
            screenDirty = true;
            return;
        }
        if (hitBox(40, 50, 200, ROW_H, tx, ty)) {
            currentScreen = Screen::ServiceIO;
            screenDirty = true;
            return;
        }
        if (hitBox(40, 80, 200, ROW_H, tx, ty)) {
            currentScreen = Screen::PinList;
            screenDirty = true;
            return;
        }
        if (hitBox(40, 110, 200, ROW_H, tx, ty)) {
            currentScreen = Screen::ReservedList;
            screenDirty = true;
            return;
        }
        if (hitBox(40, 140, 200, ROW_H, tx, ty)) {
            currentScreen = Screen::PwmList;
            screenDirty = true;
            return;
        }
        if (hitBox(40, 170, 200, ROW_H, tx, ty)) {
            currentScreen = Screen::Log;
            screenDirty = true;
            return;
        }
        if (hitBox(40, 200, 200, ROW_H, tx, ty)) {
            currentScreen = Screen::BoardTest;
            diagAnalog = false;
            diagPage = 0;
            screenDirty = true;
            return;
        }
    } else if (currentScreen == Screen::ServiceIO) {
        if (hitBox(0, 0, 60, HEADER_H, tx, ty)) {
            currentScreen = Screen::ServiceMenu;
            screenDirty = true;
            return;
        }
        if (hitBox(380, 230, 80, 30, tx, ty)) {
            ioPage++;
            screenDirty = true;
            return;
        }
        if (hitBox(300, 230, 80, 30, tx, ty)) {
            if (ioPage > 0) ioPage--;
            screenDirty = true;
            return;
        }
    } else if (currentScreen == Screen::PinList) {
        if (hitBox(0, 0, 60, HEADER_H, tx, ty)) {
            currentScreen = Screen::ServiceMenu;
            screenDirty = true;
            return;
        }
        if (hitBox(180, 230, 110, 30, tx, ty)) {
            PinMap::resetDefaults();
            PinMap::save();
            PinMap::markDirty();
            snprintf(statusMsg, sizeof(statusMsg), "Defaults restored");
            statusIsError = false;
            logStatus("Defaults restored", false);
            screenDirty = true;
            return;
        }
        if (hitBox(380, 230, 80, 30, tx, ty)) {
            pinPage++;
            screenDirty = true;
            return;
        }
        if (hitBox(300, 230, 80, 30, tx, ty)) {
            if (pinPage > 0) pinPage--;
            screenDirty = true;
            return;
        }
        // Tap on a row to edit
        int startY = HEADER_H + 10;
        int perPage = 9;
        int total = PinMap::kAnalogCount + PinMap::kDigitalCount + PinMap::kOutputCount;
        int start = pinPage * perPage;
        for (int i = 0; i < perPage; ++i) {
            int idx = start + i;
            if (idx >= total) break;
            int rowY = startY + i * ROW_H;
            if (hitBox(10, rowY, 460, ROW_H, tx, ty)) {
                if (idx < PinMap::kAnalogCount) {
                    editType = EditType::Analog;
                    editingIndex = idx;
                    editValue = PinMap::analogInputByIndex(idx);
                    editOriginal = editValue;
                    statusMsg[0] = '\0';
                } else if (idx < (PinMap::kAnalogCount + PinMap::kDigitalCount)) {
                    editType = EditType::Digital;
                    editingIndex = idx - PinMap::kAnalogCount;
                    editValue = PinMap::digitalInputByIndex(editingIndex);
                    editOriginal = editValue;
                    statusMsg[0] = '\0';
                } else {
                    editType = EditType::Output;
                    editingIndex = idx - PinMap::kAnalogCount - PinMap::kDigitalCount;
                    editValue = PinMap::outputByIndex(editingIndex);
                    editOriginal = editValue;
                    statusMsg[0] = '\0';
                }
                currentScreen = Screen::PinEdit;
                screenDirty = true;
                return;
            }
        }
    } else if (currentScreen == Screen::PinEdit) {
        if (hitBox(0, 0, 60, HEADER_H, tx, ty)) {
            currentScreen = Screen::PinList;
            screenDirty = true;
            return;
        }
        if (hitBox(80, 80, 60, 30, tx, ty)) {
            editValue--;
            statusMsg[0] = '\0';
            screenDirty = true;
            return;
        }
        if (hitBox(200, 80, 60, 30, tx, ty)) {
            editValue++;
            statusMsg[0] = '\0';
            screenDirty = true;
            return;
        }
        if (hitBox(320, 210, 120, 30, tx, ty)) {
            // Save with validation
            bool isAnalog = (editType == EditType::Analog);
            bool isDigital = (editType == EditType::Digital);

            if (PinMap::isReservedPin(editValue)) {
                snprintf(statusMsg, sizeof(statusMsg), "Error: Reserved pin");
                statusIsError = true;
                logStatus("Error: Reserved pin", true);
                screenDirty = true;
                return;
            }
            if (isAnalog && !PinMap::isAnalogCapablePin(editValue)) {
                snprintf(statusMsg, sizeof(statusMsg), "Error: Not analog pin");
                statusIsError = true;
                logStatus("Error: Not analog pin", true);
                screenDirty = true;
                return;
            }
            if (editType == EditType::Output) {
                auto id = static_cast<PinMap::OutputId>(editingIndex);
                if (PinMap::isPwmRequiredOutput(id) && !PinMap::isPwmCapablePin(editValue)) {
                    snprintf(statusMsg, sizeof(statusMsg), "Error: Not PWM pin");
                    statusIsError = true;
                    logStatus("Error: Not PWM pin", true);
                    screenDirty = true;
                    return;
                }
            }
            if (isAnalog && PinMap::wouldConflictAnalog(editingIndex, editValue)) {
                snprintf(statusMsg, sizeof(statusMsg), "Error: Pin conflict");
                statusIsError = true;
                logStatus("Error: Pin conflict", true);
                screenDirty = true;
                return;
            }
            if (!isAnalog && PinMap::wouldConflict(editType == EditType::Output, editingIndex, editValue)) {
                snprintf(statusMsg, sizeof(statusMsg), "Error: Pin conflict");
                statusIsError = true;
                logStatus("Error: Pin conflict", true);
                screenDirty = true;
                return;
            }

            if (editType == EditType::Output) {
                auto id = static_cast<PinMap::OutputId>(editingIndex);
                if (PinMap::isCriticalOutput(id)) {
                    pendingCritical = true;
                    currentScreen = Screen::ConfirmCritical;
                    screenDirty = true;
                    return;
                }
            }

            if (isAnalog) {
                PinMap::setAnalogInput(static_cast<PinMap::AnalogInputId>(editingIndex), editValue);
            } else if (isDigital) {
                PinMap::setDigitalInput(static_cast<PinMap::DigitalInputId>(editingIndex), editValue);
            } else if (editType == EditType::Output) {
                PinMap::setOutput(static_cast<PinMap::OutputId>(editingIndex), editValue);
            }

            PinMap::save();
            PinMap::markDirty();
            snprintf(statusMsg, sizeof(statusMsg), "Saved");
            statusIsError = false;
            logStatus("Saved pin mapping", false);
            currentScreen = Screen::PinList;
            screenDirty = true;
            return;
        }
        if (hitBox(180, 210, 120, 30, tx, ty)) {
            // Cancel
            currentScreen = Screen::PinList;
            screenDirty = true;
            return;
        }
    } else if (currentScreen == Screen::ConfirmCritical) {
        if (hitBox(100, 200, 100, 30, tx, ty)) {
            // Cancel
            pendingCritical = false;
            currentScreen = Screen::PinEdit;
            screenDirty = true;
            return;
        }
        if (hitBox(260, 200, 120, 30, tx, ty)) {
            // Confirm save
            pendingCritical = false;
            auto id = static_cast<PinMap::OutputId>(editingIndex);
            PinMap::setOutput(id, editValue);
            PinMap::save();
            PinMap::markDirty();
            snprintf(statusMsg, sizeof(statusMsg), "Saved");
            statusIsError = false;
            logStatus("Saved critical output", false);
            currentScreen = Screen::PinList;
            screenDirty = true;
            return;
        }
    } else if (currentScreen == Screen::ReservedList || currentScreen == Screen::PwmList) {
        if (hitBox(0, 0, 60, HEADER_H, tx, ty)) {
            currentScreen = Screen::ServiceMenu;
            screenDirty = true;
            return;
        }
        if (hitBox(10, 230, 60, 30, tx, ty)) {
            if (currentScreen == Screen::ReservedList) {
                PinRules::clearReserved();
                PinRules::save();
                logStatus("Reserved cleared", false);
            } else {
                PinRules::clearPwm();
                PinRules::save();
                logStatus("PWM cleared", false);
            }
            screenDirty = true;
            return;
        }
        if (hitBox(300, 230, 80, 30, tx, ty)) {
            if (rulePage > 0) rulePage--;
            screenDirty = true;
            return;
        }
        if (hitBox(380, 230, 80, 30, tx, ty)) {
            rulePage++;
            screenDirty = true;
            return;
        }
        if (hitBox(180, 230, 110, 30, tx, ty)) {
            // Add
            bool ok = (currentScreen == Screen::ReservedList) ? PinRules::addReserved(0) : PinRules::addPwm(0);
            if (ok) {
                PinRules::save();
                logStatus("Rule added", false);
                screenDirty = true;
            } else {
                logStatus("Rule list full", true);
            }
            return;
        }
        if (hitBox(80, 230, 90, 30, tx, ty)) {
            // Remove last
            bool ok = false;
            if (currentScreen == Screen::ReservedList && PinRules::reservedCount() > 0) {
                ok = PinRules::removeReservedAt(PinRules::reservedCount() - 1);
            } else if (currentScreen == Screen::PwmList && PinRules::pwmCount() > 0) {
                ok = PinRules::removePwmAt(PinRules::pwmCount() - 1);
            }
            if (ok) {
                PinRules::save();
                logStatus("Rule removed", false);
                screenDirty = true;
            }
            return;
        }
        // Tap a row to edit
        int startY = HEADER_H + 10;
        int perPage = 9;
        int total = (currentScreen == Screen::ReservedList) ? PinRules::reservedCount() : PinRules::pwmCount();
        int start = rulePage * perPage;
        for (int i = 0; i < perPage; ++i) {
            int idx = start + i;
            if (idx >= total) break;
            int rowY = startY + i * ROW_H;
            if (hitBox(10, rowY, 460, ROW_H, tx, ty)) {
                ruleIndex = idx;
                if (currentScreen == Screen::ReservedList) {
                    editType = EditType::Reserved;
                    editValue = PinRules::reservedAt(idx);
                } else {
                    editType = EditType::Pwm;
                    editValue = PinRules::pwmAt(idx);
                }
                editOriginal = editValue;
                currentScreen = Screen::RuleEdit;
                screenDirty = true;
                return;
            }
        }
    } else if (currentScreen == Screen::RuleEdit) {
        if (hitBox(0, 0, 60, HEADER_H, tx, ty)) {
            currentScreen = (editType == EditType::Reserved) ? Screen::ReservedList : Screen::PwmList;
            screenDirty = true;
            return;
        }
        if (hitBox(100, 140, 80, 30, tx, ty)) {
            // Label cycle
            const char* reservedPresets[] = { "SPI", "I2C", "TOUCH", "SER", "LCD", "AUX", "RESV", "" };
            const char* pwmPresets[] = { "PWM1", "PWM2", "PWM3", "FAN", "LED", "AUX", "" };
            const char** presets = (editType == EditType::Reserved) ? reservedPresets : pwmPresets;
            const int presetCount = (editType == EditType::Reserved) ? (sizeof(reservedPresets) / sizeof(reservedPresets[0])) : (sizeof(pwmPresets) / sizeof(pwmPresets[0]));
            const char* current = (editType == EditType::Reserved) ? PinRules::reservedLabelAt(ruleIndex) : PinRules::pwmLabelAt(ruleIndex);
            int nextIndex = 0;
            for (int i = 0; i < presetCount; ++i) {
                if (strcmp(presets[i], current) == 0) {
                    nextIndex = (i + 1) % presetCount;
                    break;
                }
            }
            const char* preset = presets[nextIndex];
            bool ok = (editType == EditType::Reserved) ?
                PinRules::setReservedLabelAt(ruleIndex, preset) :
                PinRules::setPwmLabelAt(ruleIndex, preset);
            if (ok) {
                PinRules::save();
                logStatus("Label set", false);
            }
            screenDirty = true;
            return;
        }
        if (hitBox(200, 140, 80, 30, tx, ty)) {
            // Type label
            kbLen = 0;
            const char* current = (editType == EditType::Reserved) ? PinRules::reservedLabelAt(ruleIndex) : PinRules::pwmLabelAt(ruleIndex);
            strncpy(kbBuffer, current, sizeof(kbBuffer) - 1);
            kbBuffer[sizeof(kbBuffer) - 1] = '\0';
            kbLen = static_cast<int>(strlen(kbBuffer));
            kbVisible = true;
            screenDirty = true;
            return;
        }
        if (hitBox(80, 80, 60, 30, tx, ty)) {
            editValue--;
            screenDirty = true;
            return;
        }
        if (hitBox(200, 80, 60, 30, tx, ty)) {
            editValue++;
            screenDirty = true;
            return;
        }
        if (hitBox(320, 210, 120, 30, tx, ty)) {
            bool ok = false;
            if (editType == EditType::Reserved) {
                ok = PinRules::setReservedAt(ruleIndex, editValue);
            } else if (editType == EditType::Pwm) {
                ok = PinRules::setPwmAt(ruleIndex, editValue);
            }
            if (ok) {
                PinRules::save();
                logStatus("Rule saved", false);
            } else {
                logStatus("Rule save failed", true);
            }
            currentScreen = (editType == EditType::Reserved) ? Screen::ReservedList : Screen::PwmList;
            screenDirty = true;
            return;
        }
        if (hitBox(180, 210, 120, 30, tx, ty)) {
            currentScreen = (editType == EditType::Reserved) ? Screen::ReservedList : Screen::PwmList;
            screenDirty = true;
            return;
        }
    }
    else if (currentScreen == Screen::TouchTest) {
        if (hitBox(0, 0, 60, HEADER_H, tx, ty)) {
            currentScreen = Screen::ServiceMenu;
            screenDirty = true;
            return;
        }
    } else if (currentScreen == Screen::BoardTest) {
        if (hitBox(0, 0, 60, HEADER_H, tx, ty)) {
            currentScreen = Screen::ServiceMenu;
            screenDirty = true;
            return;
        }
        if (hitBox(300, 0, 80, HEADER_H, tx, ty)) {
            diagAnalog = false;
            diagPage = 0;
            screenDirty = true;
            return;
        }
        if (hitBox(390, 0, 80, HEADER_H, tx, ty)) {
            diagAnalog = true;
            diagPage = 0;
            screenDirty = true;
            return;
        }
        if (!diagAnalog) {
            if (hitBox(300, 230, 80, 30, tx, ty)) {
                if (diagPage > 0) diagPage--;
                screenDirty = true;
                return;
            }
            if (hitBox(380, 230, 80, 30, tx, ty)) {
                diagPage++;
                screenDirty = true;
                return;
            }
        }
    }
}

void DisplayManager::render() {
    screenDirty = false;
    tft.fillScreen(RA8875_BLACK);
    switch (currentScreen) {
        case Screen::LiveMonitor:
            renderLiveMonitor();
            break;
        case Screen::ServiceMenu:
            renderServiceMenu();
            break;
        case Screen::ServiceIO:
            renderServiceIO();
            break;
        case Screen::PinList:
            renderPinList();
            break;
        case Screen::PinEdit:
            renderPinEdit();
            break;
        case Screen::ReservedList:
            renderReservedList();
            break;
        case Screen::PwmList:
            renderPwmList();
            break;
        case Screen::Log:
            renderLog();
            break;
        case Screen::ConfirmCritical:
            renderConfirmCritical();
            break;
        case Screen::RuleEdit:
            renderRuleEdit();
            break;
        case Screen::BoardTest:
            renderBoardTest();
            break;
        case Screen::TouchTest:
            renderTouchTest();
            break;
    }
    renderKeyboard();
}

void DisplayManager::renderLiveMonitor() {
    drawHeader("LIVE I/O MONITOR");
    drawButton(400, 0, 80, HEADER_H, "SERVICE", true);

    int row = HEADER_H + 8;
    drawStatusGrid(5, row, "Ignition ON", PinMap::digitalInput(PinMap::DigitalInputId::IgnitionOn), true); row += ROW_H;
    drawStatusGrid(5, row, "Brake Pedal", PinMap::digitalInput(PinMap::DigitalInputId::BrakePedal), true); row += ROW_H;
    drawStatusGrid(5, row, "Handbrake", PinMap::digitalInput(PinMap::DigitalInputId::Handbrake), true); row += ROW_H;
    drawStatusGrid(5, row, "Gear D", PinMap::digitalInput(PinMap::DigitalInputId::GearDrive), true); row += ROW_H + 4;

    drawStatusGrid(5, row, "Main Contactor", PinMap::output(PinMap::OutputId::MainContactor), false); row += ROW_H;
    drawStatusGrid(5, row, "Brake Lights", PinMap::output(PinMap::OutputId::BrakeLightsRelay), false); row += ROW_H + 4;

    drawStatusGrid(5, row, "Ind Left", PinMap::output(PinMap::OutputId::IndLeftRelay), false); row += ROW_H;
    drawStatusGrid(5, row, "Ind Right", PinMap::output(PinMap::OutputId::IndRightRelay), false); row += ROW_H;
}

void DisplayManager::renderServiceMenu() {
    drawHeader("SERVICE MENU");
    drawButton(0, 0, 60, HEADER_H, "BACK", false);
    drawButton(40, 50, 200, ROW_H, "I/O Monitor", true);
    drawButton(40, 80, 200, ROW_H, "Pin Remap", true);
    drawButton(40, 110, 200, ROW_H, "Reserved Pins", true);
    drawButton(40, 140, 200, ROW_H, "PWM Pins", true);
    drawButton(40, 170, 200, ROW_H, "Log", true);
    drawButton(40, 200, 200, ROW_H, "Board Test", true);
}

void DisplayManager::renderServiceIO() {
    drawHeader("SERVICE I/O");
    drawButton(0, 0, 60, HEADER_H, "BACK", false);

    const int perPage = 8;
    const int total = PinMap::kDigitalCount + PinMap::kOutputCount;
    const int start = ioPage * perPage;
    int row = HEADER_H + 8;

    for (int i = 0; i < perPage; ++i) {
        int idx = start + i;
        if (idx >= total) break;
        if (idx < PinMap::kDigitalCount) {
            auto id = static_cast<PinMap::DigitalInputId>(idx);
            drawStatusGrid(5, row, PinMap::digitalInputLabel(id), PinMap::digitalInput(id), true);
        } else {
            auto id = static_cast<PinMap::OutputId>(idx - PinMap::kDigitalCount);
            drawStatusGrid(5, row, PinMap::outputLabel(id), PinMap::output(id), false);
        }
        row += ROW_H;
    }

    drawButton(300, 230, 80, 30, "PREV", false);
    drawButton(380, 230, 80, 30, "NEXT", false);
}

void DisplayManager::renderPinList() {
    drawHeader("PIN REMAP");
    drawButton(0, 0, 60, HEADER_H, "BACK", false);

    const int perPage = 9;
    const int total = PinMap::kAnalogCount + PinMap::kDigitalCount + PinMap::kOutputCount;
    const int start = pinPage * perPage;
    int row = HEADER_H + 10;

    for (int i = 0; i < perPage; ++i) {
        int idx = start + i;
        if (idx >= total) break;
        tft.setCursor(12, row);
        tft.setTextColor(RA8875_WHITE);
        if (idx < PinMap::kAnalogCount) {
            tft.print("[ANA] Accelerator = ");
            int pin = PinMap::analogInputByIndex(idx);
            if (pin >= A0) {
                tft.print("A");
                tft.print(pin - A0);
                tft.print(" (D");
                tft.print(pin);
                tft.print(")");
            } else {
                tft.print("D");
                tft.print(pin);
            }
        } else if (idx < (PinMap::kAnalogCount + PinMap::kDigitalCount)) {
            auto id = static_cast<PinMap::DigitalInputId>(idx - PinMap::kAnalogCount);
            tft.print("[IN ] ");
            tft.print(PinMap::digitalInputLabel(id));
            tft.print(" = D");
            tft.print(PinMap::digitalInput(id));
        } else {
            auto id = static_cast<PinMap::OutputId>(idx - PinMap::kAnalogCount - PinMap::kDigitalCount);
            tft.print("[OUT] ");
            tft.print(PinMap::outputLabel(id));
            tft.print(" = D");
            tft.print(PinMap::output(id));
        }
        row += ROW_H;
    }

    if (statusMsg[0] != '\0') {
        tft.setCursor(12, 210);
        tft.setTextColor(statusIsError ? RA8875_RED : RA8875_GREEN);
        tft.print(statusMsg);
    }

    drawButton(180, 230, 110, 30, "DEFAULTS", false);
    drawButton(300, 230, 80, 30, "PREV", false);
    drawButton(380, 230, 80, 30, "NEXT", false);
}

void DisplayManager::renderPinEdit() {
    drawHeader("EDIT PIN");
    drawButton(0, 0, 60, HEADER_H, "BACK", false);

    tft.setCursor(10, 40);
    tft.setTextColor(RA8875_WHITE);
    bool isAnalog = (editType == EditType::Analog);
    bool isDigital = (editType == EditType::Digital);
    if (editType == EditType::Output) {
        auto id = static_cast<PinMap::OutputId>(editingIndex);
        tft.print("Output: ");
        tft.print(PinMap::outputLabel(id));
    } else if (isAnalog) {
        tft.print("Analog: Accelerator");
    } else if (isDigital) {
        auto id = static_cast<PinMap::DigitalInputId>(editingIndex);
        tft.print("Input: ");
        tft.print(PinMap::digitalInputLabel(id));
    }

    tft.setCursor(10, 65);
    if (isAnalog && editValue >= A0) {
        tft.print("Pin: A");
        tft.print(editValue - A0);
        tft.print(" (D");
        tft.print(editValue);
        tft.print(")");
    } else {
        tft.print("Pin: D");
        tft.print(editValue);
    }

    if (PinMap::isReservedPin(editValue)) {
        tft.setCursor(10, 100);
        tft.setTextColor(RA8875_RED);
        tft.print("Reserved pin");
    } else if (isAnalog && !PinMap::isAnalogCapablePin(editValue)) {
        tft.setCursor(10, 100);
        tft.setTextColor(RA8875_RED);
        tft.print("Not analog-capable");
    } else if (editType == EditType::Output) {
        auto id = static_cast<PinMap::OutputId>(editingIndex);
        if (PinMap::isPwmRequiredOutput(id) && !PinMap::isPwmCapablePin(editValue)) {
            tft.setCursor(10, 100);
            tft.setTextColor(RA8875_RED);
            tft.print("Not PWM-capable");
        }
    }

    if (statusMsg[0] != '\0') {
        tft.setCursor(10, 130);
        tft.setTextColor(statusIsError ? RA8875_RED : RA8875_GREEN);
        tft.print(statusMsg);
    }

    drawButton(80, 80, 60, 30, "-", true);
    drawButton(200, 80, 60, 30, "+", true);
    drawButton(180, 210, 120, 30, "CANCEL", false);
    drawButton(320, 210, 120, 30, "SAVE", true);
}

void DisplayManager::renderReservedList() {
    drawHeader("RESERVED PINS");
    drawButton(0, 0, 60, HEADER_H, "BACK", false);

    const int perPage = 9;
    const int total = PinRules::reservedCount();
    const int start = rulePage * perPage;
    int row = HEADER_H + 10;
    for (int i = 0; i < perPage; ++i) {
        int idx = start + i;
        if (idx >= total) break;
        tft.setCursor(12, row);
        tft.setTextColor(RA8875_WHITE);
        tft.print("D");
        tft.print(PinRules::reservedAt(idx));
        const char* label = PinRules::reservedLabelAt(idx);
        if (label[0] != '\0') {
            tft.print(" ");
            tft.print(label);
        }
        row += ROW_H;
    }

    drawButton(10, 230, 60, 30, "CLR", false);
    drawButton(80, 230, 90, 30, "DEL", false);
    drawButton(180, 230, 110, 30, "ADD", false);
    drawButton(300, 230, 80, 30, "PREV", false);
    drawButton(380, 230, 80, 30, "NEXT", false);
}

void DisplayManager::renderPwmList() {
    drawHeader("PWM PINS");
    drawButton(0, 0, 60, HEADER_H, "BACK", false);

    const int perPage = 9;
    const int total = PinRules::pwmCount();
    const int start = rulePage * perPage;
    int row = HEADER_H + 10;
    for (int i = 0; i < perPage; ++i) {
        int idx = start + i;
        if (idx >= total) break;
        tft.setCursor(12, row);
        tft.setTextColor(RA8875_WHITE);
        tft.print("D");
        tft.print(PinRules::pwmAt(idx));
        const char* label = PinRules::pwmLabelAt(idx);
        if (label[0] != '\0') {
            tft.print(" ");
            tft.print(label);
        }
        row += ROW_H;
    }

    drawButton(10, 230, 60, 30, "CLR", false);
    drawButton(80, 230, 90, 30, "DEL", false);
    drawButton(180, 230, 110, 30, "ADD", false);
    drawButton(300, 230, 80, 30, "PREV", false);
    drawButton(380, 230, 80, 30, "NEXT", false);
}

void DisplayManager::renderLog() {
    drawHeader("SERVICE LOG");
    drawButton(0, 0, 60, HEADER_H, "BACK", false);

    int row = HEADER_H + 10;
    int show = (logCount < 9) ? logCount : 9;
    for (int i = 0; i < show; ++i) {
        int idx = (logHead - show + i + 10) % 10;
        if (logMsgs[idx][0] == '\0') continue;
        tft.setCursor(12, row);
        tft.setTextColor(RA8875_WHITE);
        tft.print(logMsgs[idx]);
        row += ROW_H;
    }
}

void DisplayManager::renderConfirmCritical() {
    drawHeader("CONFIRM CHANGE");
    tft.setCursor(10, 60);
    tft.setTextColor(RA8875_RED);
    tft.print("Critical output change!");
    tft.setCursor(10, 80);
    tft.setTextColor(RA8875_WHITE);
    tft.print("Are you sure?");
    drawButton(100, 200, 100, 30, "CANCEL", false);
    drawButton(260, 200, 120, 30, "CONFIRM", true);
}

void DisplayManager::renderRuleEdit() {
    drawHeader("EDIT RULE");
    drawButton(0, 0, 60, HEADER_H, "BACK", false);

    tft.setCursor(10, 40);
    tft.setTextColor(RA8875_WHITE);
    if (editType == EditType::Reserved) {
        tft.print("Reserved pin");
    } else {
        tft.print("PWM pin");
    }

    tft.setCursor(10, 65);
    tft.print("Pin: D");
    tft.print(editValue);

    tft.setCursor(10, 110);
    if (editType == EditType::Reserved) {
        tft.print("Label: ");
        tft.print(PinRules::reservedLabelAt(ruleIndex));
    } else {
        tft.print("Label: ");
        tft.print(PinRules::pwmLabelAt(ruleIndex));
    }
    tft.setCursor(10, 125);
    tft.print("Tap LABEL or TYPE");

    drawButton(80, 80, 60, 30, "-", true);
    drawButton(200, 80, 60, 30, "+", true);
    drawButton(100, 140, 80, 30, "LABEL", false);
    drawButton(200, 140, 80, 30, "TYPE", false);
    drawButton(180, 210, 120, 30, "CANCEL", false);
    drawButton(320, 210, 120, 30, "SAVE", true);
}

void DisplayManager::renderBoardTest() {
    drawHeader("BOARD TEST");
    drawButton(0, 0, 60, HEADER_H, "BACK", false);
    drawButton(300, 0, 80, HEADER_H, "DIG", !diagAnalog);
    drawButton(390, 0, 80, HEADER_H, "ANA", diagAnalog);

    if (diagAnalog) {
        int row = HEADER_H + 8;
        for (int i = 0; i < 12; ++i) {
            const int val = analogRead(A0 + i);
            tft.setCursor(10, row);
            tft.setTextColor(RA8875_WHITE);
            tft.print("A");
            tft.print(i);
            tft.print(" (D");
            tft.print(A0 + i);
            tft.print("): ");
            tft.print(val);
            row += ROW_H;
        }
        return;
    }

    constexpr int kTotalPins = 66;
    constexpr int kRows = 10;
    constexpr int kCols = 2;
    constexpr int kPinsPerPage = kRows * kCols;
    const int maxPage = (kTotalPins - 1) / kPinsPerPage;
    if (diagPage > maxPage) diagPage = maxPage;

    for (int i = 0; i < kPinsPerPage; ++i) {
        const int pin = diagPage * kPinsPerPage + i;
        if (pin >= kTotalPins) break;
        const int col = i / kRows;
        const int rowIdx = i % kRows;
        const int x = 10 + col * 235;
        const int y = HEADER_H + 8 + rowIdx * 20;
        const int state = digitalRead(pin);

        tft.setCursor(x, y);
        tft.setTextColor(RA8875_WHITE);
        tft.print("D");
        if (pin < 10) tft.print("0");
        tft.print(pin);
        tft.print(": ");
        tft.setTextColor(state == HIGH ? RA8875_GREEN : RA8875_RED);
        tft.print(state == HIGH ? "HIGH" : "LOW ");
    }

    tft.setCursor(175, 235);
    tft.setTextColor(RA8875_WHITE);
    tft.print("Page ");
    tft.print(diagPage + 1);
    tft.print("/");
    tft.print(maxPage + 1);

    drawButton(300, 230, 80, 30, "PREV", false);
    drawButton(380, 230, 80, 30, "NEXT", false);
}

void DisplayManager::renderTouchTest() {
    drawHeader("TOUCH TEST");
    drawButton(0, 0, 60, HEADER_H, "BACK", false);

    tft.setCursor(10, 40);
    tft.setTextColor(RA8875_WHITE);
    tft.print("Touch: ");
    tft.print(touchActive ? "YES" : "NO");

    tft.setCursor(10, 60);
    tft.print("X: ");
    tft.print(touchX);
    tft.print("  Y: ");
    tft.print(touchY);

    if (touchActive) {
        tft.fillCircle(touchX, touchY, 3, RA8875_GREEN);
    }
}

void DisplayManager::renderKeyboard() {
    if (!kbVisible) return;
    const int kbY = 150;
    const int keyW = 40;
    const int keyH = 28;

    tft.fillRect(0, kbY, SCREEN_W, SCREEN_H - kbY, RA8875_BLACK);
    tft.drawRect(0, kbY, SCREEN_W, SCREEN_H - kbY, RA8875_WHITE);

    const char* row1 = "1234567890";
    const char* row2 = "QWERTYUIOP";
    const char* row3 = "ASDFGHJKL";
    const char* row4 = "ZXCVBNM";

    for (int i = 0; i < 10; ++i) {
        tft.setCursor(i * keyW + 8, kbY + 6);
        tft.print(row1[i]);
    }
    for (int i = 0; i < 10; ++i) {
        tft.setCursor(i * keyW + 8, kbY + keyH + 6);
        tft.print(row2[i]);
    }
    for (int i = 0; i < 9; ++i) {
        tft.setCursor(i * keyW + 8, kbY + keyH * 2 + 6);
        tft.print(row3[i]);
    }
    for (int i = 0; i < 7; ++i) {
        tft.setCursor(i * keyW + 8, kbY + keyH * 3 + 6);
        tft.print(row4[i]);
    }
    tft.setCursor(7 * keyW + 6, kbY + keyH * 3 + 6);
    tft.print("_");
    tft.setCursor(8 * keyW + 6, kbY + keyH * 3 + 6);
    tft.print("<");
    tft.setCursor(9 * keyW + 6, kbY + keyH * 3 + 6);
    tft.print("OK");

    tft.setCursor(10, kbY - 12);
    tft.setTextColor(RA8875_WHITE);
    tft.print("Label: ");
    tft.print(kbBuffer);
}

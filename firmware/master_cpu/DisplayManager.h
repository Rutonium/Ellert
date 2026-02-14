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
    void drawButton(int x, int y, int w, int h, const char* label, bool filled);
    void drawHeader(const char* title);
    void logStatus(const char* msg, bool isError);
    void handleTouch();
    void render();

    void renderLiveMonitor();
    void renderServiceMenu();
    void renderServiceIO();
    void renderPinList();
    void renderPinEdit();
    void renderReservedList();
    void renderPwmList();
    void renderLog();
    void renderConfirmCritical();
    void renderRuleEdit();
    void renderBoardTest();
    void renderTouchTest();
    void renderKeyboard();

    void touchToService();
    bool hitBox(int x, int y, int w, int h, int tx, int ty) const;

    enum class Screen {
        LiveMonitor = 0,
        ServiceMenu,
        ServiceIO,
        PinList,
        PinEdit,
        ReservedList,
        PwmList,
        Log,
        ConfirmCritical,
        RuleEdit,
        BoardTest,
        TouchTest
    };

    enum class EditType {
        Analog = 0,
        Digital,
        Output,
        Reserved,
        Pwm
    };

    Screen currentScreen = Screen::LiveMonitor;
    bool screenDirty = true;
    int ioPage = 0;
    int pinPage = 0;
    EditType editType = EditType::Digital;
    int editingIndex = 0;
    int editValue = 0;
    int editOriginal = 0;
    unsigned long lastTouchMs = 0;

    char statusMsg[48] = {0};
    bool statusIsError = false;

    char logMsgs[10][48] = {{0}};
    int logHead = 0;
    int logCount = 0;

    int rulePage = 0;
    int ruleIndex = -1;

    bool pendingCritical = false;

    int touchX = 0;
    int touchY = 0;
    bool touchActive = false;

    int diagPage = 0;
    bool diagAnalog = false;
    bool displayAvailable = false;

    bool kbVisible = false;
    char kbBuffer[13] = {0};
    int kbLen = 0;
};
#endif // DISPLAY_MANAGER_H

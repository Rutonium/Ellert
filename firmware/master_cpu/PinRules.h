#ifndef PIN_RULES_H
#define PIN_RULES_H

#include <Arduino.h>

class PinRules {
public:
    static void load();
    static void save();
    static void resetDefaults();

    static int reservedCount();
    static int pwmCount();

    static int reservedAt(int index);
    static int pwmAt(int index);
    static const char* reservedLabelAt(int index);
    static const char* pwmLabelAt(int index);

    static bool addReserved(int pin);
    static bool addPwm(int pin);
    static bool setReservedAt(int index, int pin);
    static bool setPwmAt(int index, int pin);
    static bool setReservedLabelAt(int index, const char* label);
    static bool setPwmLabelAt(int index, const char* label);
    static bool removeReservedAt(int index);
    static bool removePwmAt(int index);
    static void clearReserved();
    static void clearPwm();

    static bool isReserved(int pin);
    static bool isPwmAllowed(int pin);

private:
    static void applyDefaults();
};

#endif // PIN_RULES_H

#include "PinRules.h"
#include "EEPROM.h"

namespace {
    constexpr uint32_t kMagic = 0x50524C53; // 'PRLS'
    constexpr uint16_t kVersion = 1;
    constexpr int kMax = 16;
    constexpr int kLabelLen = 12;
    constexpr int kEepromOffset = 512;

    struct RulesData {
        uint32_t magic;
        uint16_t version;
        uint16_t checksum;
        int16_t reservedCount;
        int16_t pwmCount;
        int16_t reserved[kMax];
        int16_t pwm[kMax];
        char reservedLabel[kMax][kLabelLen];
        char pwmLabel[kMax][kLabelLen];
    };

    RulesData g_rules;

    uint16_t checksum(const RulesData& data) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&data);
        const size_t len = sizeof(RulesData) - sizeof(uint16_t);
        uint16_t sum = 0;
        for (size_t i = 0; i < len; ++i) sum = static_cast<uint16_t>(sum + bytes[i]);
        return sum;
    }
}

void PinRules::applyDefaults() {
    g_rules.magic = kMagic;
    g_rules.version = kVersion;
    g_rules.reservedCount = 0;
    g_rules.pwmCount = 0;
    for (int i = 0; i < kMax; ++i) {
        g_rules.reserved[i] = -1;
        g_rules.pwm[i] = -1;
        g_rules.reservedLabel[i][0] = '\0';
        g_rules.pwmLabel[i][0] = '\0';
    }
    g_rules.checksum = checksum(g_rules);
}

void PinRules::resetDefaults() {
    applyDefaults();
}

void PinRules::load() {
    EEPROM.get(kEepromOffset, g_rules);
    if (g_rules.magic != kMagic || g_rules.version != kVersion) {
        applyDefaults();
        save();
        return;
    }
    if (checksum(g_rules) != g_rules.checksum) {
        applyDefaults();
        save();
    }
}

void PinRules::save() {
    g_rules.checksum = checksum(g_rules);
    EEPROM.put(kEepromOffset, g_rules);
}

int PinRules::reservedCount() { return g_rules.reservedCount; }
int PinRules::pwmCount() { return g_rules.pwmCount; }

int PinRules::reservedAt(int index) {
    if (index < 0 || index >= g_rules.reservedCount) return -1;
    return g_rules.reserved[index];
}

int PinRules::pwmAt(int index) {
    if (index < 0 || index >= g_rules.pwmCount) return -1;
    return g_rules.pwm[index];
}

const char* PinRules::reservedLabelAt(int index) {
    if (index < 0 || index >= g_rules.reservedCount) return "";
    return g_rules.reservedLabel[index];
}

const char* PinRules::pwmLabelAt(int index) {
    if (index < 0 || index >= g_rules.pwmCount) return "";
    return g_rules.pwmLabel[index];
}

bool PinRules::addReserved(int pin) {
    if (g_rules.reservedCount >= kMax) return false;
    for (int i = 0; i < g_rules.reservedCount; ++i) {
        if (g_rules.reserved[i] == pin) return false;
    }
    g_rules.reserved[g_rules.reservedCount++] = static_cast<int16_t>(pin);
    g_rules.reservedLabel[g_rules.reservedCount - 1][0] = '\0';
    return true;
}

bool PinRules::addPwm(int pin) {
    if (g_rules.pwmCount >= kMax) return false;
    for (int i = 0; i < g_rules.pwmCount; ++i) {
        if (g_rules.pwm[i] == pin) return false;
    }
    g_rules.pwm[g_rules.pwmCount++] = static_cast<int16_t>(pin);
    g_rules.pwmLabel[g_rules.pwmCount - 1][0] = '\0';
    return true;
}

bool PinRules::setReservedAt(int index, int pin) {
    if (index < 0 || index >= g_rules.reservedCount) return false;
    g_rules.reserved[index] = static_cast<int16_t>(pin);
    return true;
}

bool PinRules::setPwmAt(int index, int pin) {
    if (index < 0 || index >= g_rules.pwmCount) return false;
    g_rules.pwm[index] = static_cast<int16_t>(pin);
    return true;
}

bool PinRules::setReservedLabelAt(int index, const char* label) {
    if (index < 0 || index >= g_rules.reservedCount) return false;
    strncpy(g_rules.reservedLabel[index], label, kLabelLen - 1);
    g_rules.reservedLabel[index][kLabelLen - 1] = '\0';
    return true;
}

bool PinRules::setPwmLabelAt(int index, const char* label) {
    if (index < 0 || index >= g_rules.pwmCount) return false;
    strncpy(g_rules.pwmLabel[index], label, kLabelLen - 1);
    g_rules.pwmLabel[index][kLabelLen - 1] = '\0';
    return true;
}

bool PinRules::removeReservedAt(int index) {
    if (index < 0 || index >= g_rules.reservedCount) return false;
    for (int i = index; i < g_rules.reservedCount - 1; ++i) {
        g_rules.reserved[i] = g_rules.reserved[i + 1];
        strncpy(g_rules.reservedLabel[i], g_rules.reservedLabel[i + 1], kLabelLen);
        g_rules.reservedLabel[i][kLabelLen - 1] = '\0';
    }
    g_rules.reserved[--g_rules.reservedCount] = -1;
    g_rules.reservedLabel[g_rules.reservedCount][0] = '\0';
    return true;
}

bool PinRules::removePwmAt(int index) {
    if (index < 0 || index >= g_rules.pwmCount) return false;
    for (int i = index; i < g_rules.pwmCount - 1; ++i) {
        g_rules.pwm[i] = g_rules.pwm[i + 1];
        strncpy(g_rules.pwmLabel[i], g_rules.pwmLabel[i + 1], kLabelLen);
        g_rules.pwmLabel[i][kLabelLen - 1] = '\0';
    }
    g_rules.pwm[--g_rules.pwmCount] = -1;
    g_rules.pwmLabel[g_rules.pwmCount][0] = '\0';
    return true;
}

void PinRules::clearReserved() {
    g_rules.reservedCount = 0;
    for (int i = 0; i < kMax; ++i) {
        g_rules.reserved[i] = -1;
        g_rules.reservedLabel[i][0] = '\0';
    }
}

void PinRules::clearPwm() {
    g_rules.pwmCount = 0;
    for (int i = 0; i < kMax; ++i) {
        g_rules.pwm[i] = -1;
        g_rules.pwmLabel[i][0] = '\0';
    }
}

bool PinRules::isReserved(int pin) {
    for (int i = 0; i < g_rules.reservedCount; ++i) {
        if (g_rules.reserved[i] == pin) return true;
    }
    return false;
}

bool PinRules::isPwmAllowed(int pin) {
    if (g_rules.pwmCount == 0) return true;
    for (int i = 0; i < g_rules.pwmCount; ++i) {
        if (g_rules.pwm[i] == pin) return true;
    }
    return false;
}

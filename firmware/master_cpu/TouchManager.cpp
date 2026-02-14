#include "TouchManager.h"
#include <Wire.h>

namespace {
    constexpr uint8_t kAddrA = 0x5D; // GT9271 common address
    constexpr uint8_t kAddrB = 0x14; // Alternate address
    constexpr uint16_t kStatusReg = 0x814E;
    constexpr uint16_t kFirstPointReg = 0x814F;

    uint8_t g_addr = kAddrA;
    bool g_found = false;

    bool readBytes(uint16_t reg, uint8_t* buf, size_t len) {
        Wire.beginTransmission(g_addr);
        Wire.write(static_cast<uint8_t>(reg >> 8));
        Wire.write(static_cast<uint8_t>(reg & 0xFF));
        if (Wire.endTransmission(false) != 0) return false;
        size_t read = Wire.requestFrom(g_addr, static_cast<uint8_t>(len));
        if (read != len) return false;
        for (size_t i = 0; i < len; ++i) {
            buf[i] = Wire.read();
        }
        return true;
    }

    bool writeByte(uint16_t reg, uint8_t value) {
        Wire.beginTransmission(g_addr);
        Wire.write(static_cast<uint8_t>(reg >> 8));
        Wire.write(static_cast<uint8_t>(reg & 0xFF));
        Wire.write(value);
        return Wire.endTransmission(true) == 0;
    }
}

bool TouchManager::begin() {
    Wire.begin();

    g_addr = kAddrA;
    uint8_t status = 0;
    if (readBytes(kStatusReg, &status, 1)) {
        g_found = true;
        return true;
    }

    g_addr = kAddrB;
    if (readBytes(kStatusReg, &status, 1)) {
        g_found = true;
        return true;
    }

    g_found = false;
    return false;
}

bool TouchManager::readTouch(int& x, int& y) {
    if (!g_found) return false;

    uint8_t status = 0;
    if (!readBytes(kStatusReg, &status, 1)) return false;
    const uint8_t touchCount = status & 0x0F;
    const bool ready = (status & 0x80) != 0;
    if (!ready || touchCount == 0) return false;

    uint8_t buf[8] = {0};
    if (!readBytes(kFirstPointReg, buf, sizeof(buf))) return false;

    x = static_cast<int>(buf[1] << 8 | buf[0]);
    y = static_cast<int>(buf[3] << 8 | buf[2]);

    // Clear status to acknowledge
    writeByte(kStatusReg, 0);
    return true;
}

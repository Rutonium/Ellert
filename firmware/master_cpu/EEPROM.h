#ifndef ELLERT_EEPROM_COMPAT_H
#define ELLERT_EEPROM_COMPAT_H

#include <Arduino.h>
#include <cstring>

class EEPROMClass {
public:
  static constexpr int kSize = 4096;

  template <typename T>
  T& get(int address, T& value) {
    if (!valid(address, sizeof(T))) { memset(&value, 0, sizeof(T)); return value; }
    memcpy(&value, &data_[address], sizeof(T));
    return value;
  }

  template <typename T>
  const T& put(int address, const T& value) {
    if (valid(address, sizeof(T))) memcpy(&data_[address], &value, sizeof(T));
    return value;
  }

private:
  bool valid(int address, size_t len) const {
    if (address < 0) return false;
    size_t a = (size_t)address;
    return a <= (size_t)kSize && len <= ((size_t)kSize - a);
  }

  uint8_t data_[kSize] = {0};
};

extern EEPROMClass EEPROM;

#endif

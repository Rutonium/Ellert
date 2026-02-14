#ifndef SERIAL_DIAGNOSTICS_H
#define SERIAL_DIAGNOSTICS_H

#include <Arduino.h>

class SerialDiagnostics {
public:
    static void initialize();
    static void update();
};

#endif // SERIAL_DIAGNOSTICS_H

#ifndef REMOTE_INTERFACES_H
#define REMOTE_INTERFACES_H

#include <Arduino.h>

class RemoteInterfaces {
public:
    static void initialize();
    static void update();

    static bool inputCpuOnline();
    static bool displayCpuOnline();
};

#endif // REMOTE_INTERFACES_H

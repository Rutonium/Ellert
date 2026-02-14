#ifndef TOUCH_MANAGER_H
#define TOUCH_MANAGER_H

#include <Arduino.h>

class TouchManager {
public:
    bool begin();
    bool readTouch(int& x, int& y);
};

#endif // TOUCH_MANAGER_H

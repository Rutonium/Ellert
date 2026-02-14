#ifndef ACCESSORY_CONTROL_H
#define ACCESSORY_CONTROL_H

#include <Arduino.h>

class AccessoryControl {
public:
    void initialize();
    void update();
private:
    void handleLights();
    void handleWipers();
    void handleHeating();
};
#endif // ACCESSORY_CONTROL_H

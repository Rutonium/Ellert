#ifndef INPUT_FILTER_H
#define INPUT_FILTER_H

#include <Arduino.h>

class InputFilter {
public:
    static void initialize();
    static void update(); 
    static int getDebouncedState(int pin);
};
#endif // INPUT_FILTER_H

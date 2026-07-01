#pragma once
#include <Arduino.h>

class PCA9548A
{
public:
    void begin();
    void select(uint8_t channel);
};
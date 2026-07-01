#pragma once
#include <Arduino.h>
#include <Adafruit_VL6180X.h>

class ToF {
public:
    void begin();
    void update();

    uint16_t front();
    uint16_t rear();

private:
    Adafruit_VL6180X _frontSensor;
    Adafruit_VL6180X _rearSensor;

    bool _frontOk = false;
    bool _rearOk = false;

    uint16_t _front = 0;
    uint16_t _rear = 0;
};
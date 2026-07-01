#pragma once
#include <Arduino.h>

class Motors
{
public:
    void begin();
    void update();

    void setSpeed(int left, int right); // -255 to +255
    void stop();

private:
    int _left = 0;
    int _right = 0;

    unsigned long _ledTimer = 0;
    bool          _ledState = false;

    void applyMotor(int lf, int lb, int speed);
};
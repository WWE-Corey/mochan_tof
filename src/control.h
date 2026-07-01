#pragma once
#include <Arduino.h>
#include "eyes.h"

enum ControlMode
{
    MODE_AUTO,
    MODE_MANUAL,
    MODE_CURIOUS
};

struct ControlState
{
    ControlMode mode = MODE_AUTO;

    int throttle = 0;   // -255 to 255
    int steering = 0;   // -255 to 255

    EyeMood moodOverride = MOOD_HAPPY;
    bool moodOverrideActive = false;
};

extern ControlState control;
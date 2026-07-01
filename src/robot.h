#pragma once
#include "tof.h"
#include "eyes.h"
#include "motors.h"
#include "control.h"

struct RobotState
{
    ControlMode mode = MODE_AUTO;

    bool cliffFront = false;
    bool cliffRear = false;

    int speed = 0;
    int steer = 0;
};

extern RobotState state;

enum AutoAction
{
    AUTO_PAUSE,
    AUTO_FORWARD,
    AUTO_TURN_ARC,
    AUTO_PIVOT,
    AUTO_REVERSE
};

class Robot
{
public:
    void begin(ToF* tof, Eyes* eyes, Motors* motors);
    void update();

private:
    void startCalibration();
    void calibrate();
    bool isCliff(uint16_t current, uint16_t baseline);

    void runManual();
    void runAuto();
    void runCurious();

    void startAutoAction(AutoAction action, unsigned long duration);
    void pickNextAutoAction();

    ToF* _tof;
    Eyes* _eyes;
    Motors* _motors;

    ControlMode _lastMode = MODE_MANUAL; // forces a clean entry into the boot-time mode

    bool _calibrated = false;

    uint32_t _calibStart = 0;
    uint16_t _calibSamples = 0;

    uint32_t _floorFront = 0;
    uint32_t _floorRear = 0;

    // Auto-mode random-drive state machine
    AutoAction _autoAction = AUTO_FORWARD;
    unsigned long _autoActionStart = 0;
    unsigned long _autoActionDuration = 0;
    int _autoTurnSign = 1;
    bool _evading = false; // forced reverse-then-pivot sequence after a cliff

    // Curious-mode look-around state
    unsigned long _curiousActionStart = 0;
    unsigned long _curiousActionDuration = 0;
    int _curiousTurnSign = 1;
    bool _curiousTurning = false;
};
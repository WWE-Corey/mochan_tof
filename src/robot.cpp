#include "robot.h"
#include "config.h"
#include <Arduino.h>
#include "control.h"

extern ControlState control;

RobotState state;

// Speeds are kept at or above 200 — the value already proven to reliably
// move both motors from a dead stop. Lower values left one motor's
// gearbox below its stall threshold and stationary.
static const int AUTO_SPEED   = 220;
static const int AUTO_ARC_TURN = 140;
static const int CURIOUS_SPEED = 210;

void Robot::begin(ToF* tof, Eyes* eyes, Motors* motors)
{
    _tof = tof;
    _eyes = eyes;
    _motors = motors;

    startCalibration();
}

void Robot::startCalibration()
{
    _calibrated   = false;
    _calibStart   = millis();
    _calibSamples = 0;
    _floorFront   = 0;
    _floorRear    = 0;
    _evading      = false;
}

void Robot::calibrate()
{
    uint16_t f = _tof->front();
    uint16_t r = _tof->rear();

    if (f > 0 && r > 0 && f < 250 && r < 250)
    {
        _floorFront += f;
        _floorRear += r;
        _calibSamples++;
    }

    if (millis() - _calibStart > 2000 && _calibSamples > 0)
    {
        _floorFront /= _calibSamples;
        _floorRear /= _calibSamples;

        _calibrated = true;
    }
}

bool Robot::isCliff(uint16_t current, uint16_t baseline)
{
    if (baseline == 0) return false;

    return current > (baseline + (baseline * 0.25)) || current > 180;
}

void Robot::update()
{
    // -------------------------
    // MODE TRANSITIONS
    // -------------------------
    if (control.mode != _lastMode)
    {
        if (control.mode == MODE_AUTO)
            startCalibration();

        if (control.mode == MODE_CURIOUS)
        {
            _curiousActionStart    = millis();
            _curiousActionDuration = random(700, 2000);
            _curiousTurning        = false;
            _eyes->setCuriousLook(true);
        }
        else if (_lastMode == MODE_CURIOUS)
        {
            _eyes->setCuriousLook(false);
        }

        _lastMode = control.mode;
    }

    state.mode = control.mode;

    switch (control.mode)
    {
        case MODE_MANUAL:  runManual();  break;
        case MODE_CURIOUS: runCurious(); break;
        default:           runAuto();    break; // MODE_AUTO
    }
}

// -------------------------
// MANUAL MODE
// -------------------------
void Robot::runManual()
{
    state.speed = control.throttle;
    state.steer = control.steering;

    int left  = state.speed + state.steer;
    int right = state.speed - state.steer;

    left  = constrain(left, -255, 255);
    right = constrain(right, -255, 255);

    _motors->setSpeed(left, right);

    _eyes->setMood(control.moodOverrideActive ? control.moodOverride : MOOD_HAPPY);
}

// -------------------------
// AUTO MODE — random drive with cliff avoidance
// -------------------------
void Robot::startAutoAction(AutoAction action, unsigned long duration)
{
    _autoAction         = action;
    _autoActionStart    = millis();
    _autoActionDuration = duration;
    _autoTurnSign       = (random(2) == 0) ? 1 : -1;
}

void Robot::pickNextAutoAction()
{
    int r = random(100);
    if (r < 55)       startAutoAction(AUTO_FORWARD,  random(1200, 3000));
    else if (r < 75)  startAutoAction(AUTO_TURN_ARC, random(500, 1200));
    else if (r < 90)  startAutoAction(AUTO_PIVOT,    random(400, 900));
    else              startAutoAction(AUTO_REVERSE,  random(500, 1000));
}

void Robot::runAuto()
{
    // Sit still and sample the floor for a baseline before driving, so
    // cliff detection has something real to compare against.
    if (!_calibrated)
    {
        calibrate();
        _motors->setSpeed(0, 0);
        _eyes->setMood(MOOD_IDLE);
        return;
    }

    uint16_t front = _tof->front();
    uint16_t rear  = _tof->rear();

    state.cliffFront = isCliff(front, _floorFront);
    state.cliffRear   = isCliff(rear, _floorRear);

    unsigned long now = millis();

    // Cliff evasion is a forced reverse-then-pivot sequence, not a single
    // reaction — backing straight up doesn't guarantee a wide edge actually
    // clears the front sensor, so a turn to a new heading always follows
    // before random driving is allowed to resume. Without the forced pivot,
    // the random picker could send it straight back into the same edge and
    // the safety check would just bounce it into reverse again forever.
    if (!_evading && state.cliffFront)
    {
        _evading = true;
        startAutoAction(AUTO_REVERSE, random(700, 1300));
    }
    else if (_evading && _autoAction == AUTO_REVERSE &&
             (state.cliffRear || now - _autoActionStart >= _autoActionDuration))
    {
        // Either backed up long enough, or backing up further would run
        // into a cliff behind too — either way, turn to a new heading next.
        startAutoAction(AUTO_PIVOT, random(600, 1100));
    }
    else if (_evading && _autoAction == AUTO_PIVOT && now - _autoActionStart >= _autoActionDuration)
    {
        // Evasion complete — pause before resuming normal driving.
        _evading = false;
        startAutoAction(AUTO_PAUSE, random(800, 2000));
    }
    else if (!_evading && _autoAction == AUTO_PAUSE && now - _autoActionStart >= _autoActionDuration)
    {
        pickNextAutoAction();
    }
    else if (!_evading && _autoAction != AUTO_PAUSE && now - _autoActionStart >= _autoActionDuration)
    {
        // Every movement ends with a thinking pause before the next one.
        startAutoAction(AUTO_PAUSE, random(800, 2000));
    }

    switch (_autoAction)
    {
        case AUTO_PAUSE:
            state.speed = 0;
            state.steer = 0;
            _motors->setSpeed(0, 0);
            _eyes->setMood(MOOD_IDLE);
            break;

        case AUTO_FORWARD:
            state.speed = AUTO_SPEED;
            state.steer = 0;
            _motors->setSpeed(AUTO_SPEED, AUTO_SPEED);
            _eyes->setMood(MOOD_IDLE);
            break;

        case AUTO_TURN_ARC:
        {
            int turn  = AUTO_ARC_TURN * _autoTurnSign;
            int left  = constrain(AUTO_SPEED + turn, -255, 255);
            int right = constrain(AUTO_SPEED - turn, -255, 255);
            state.speed = AUTO_SPEED;
            state.steer = turn;
            _motors->setSpeed(left, right);
            _eyes->setMood(MOOD_WARN);
            break;
        }

        case AUTO_PIVOT:
        {
            int spin = AUTO_SPEED * _autoTurnSign;
            state.speed = 0;
            state.steer = spin;
            _motors->setSpeed(spin, -spin);
            _eyes->setMood(MOOD_WARN);
            break;
        }

        case AUTO_REVERSE:
            state.speed = -AUTO_SPEED;
            state.steer = 0;
            _motors->setSpeed(-AUTO_SPEED, -AUTO_SPEED);
            _eyes->setMood(MOOD_DANGER);
            break;
    }
}

// -------------------------
// CURIOUS MODE — stay put, look around
// -------------------------
void Robot::runCurious()
{
    unsigned long now = millis();

    if (now - _curiousActionStart >= _curiousActionDuration)
    {
        _curiousActionStart = now;
        _curiousTurning = !_curiousTurning;

        if (_curiousTurning)
        {
            _curiousTurnSign       = (random(2) == 0) ? 1 : -1;
            _curiousActionDuration = random(250, 600); // brief glance turn
        }
        else
        {
            _curiousActionDuration = random(700, 2000); // pause and "look"
        }
    }

    if (_curiousTurning)
    {
        int spin = CURIOUS_SPEED * _curiousTurnSign;
        state.speed = 0;
        state.steer = spin;
        _motors->setSpeed(spin, -spin);
    }
    else
    {
        state.speed = 0;
        state.steer = 0;
        _motors->setSpeed(0, 0);
    }

    _eyes->setMood(MOOD_IDLE);
}

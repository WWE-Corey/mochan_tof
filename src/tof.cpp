#include "tof.h"
#include "pca9548a.h"
#include "config.h"

extern PCA9548A PCA;

void ToF::begin()
{
    PCA.select(PCA_VL_FRONT);
    _frontOk = _frontSensor.begin();
    if (_frontOk) _frontSensor.startRangeContinuous(50);
    else Serial.println("ToF: front VL6180X not found");

    PCA.select(PCA_VL_REAR);
    _rearOk = _rearSensor.begin();
    if (_rearOk) _rearSensor.startRangeContinuous(50);
    else Serial.println("ToF: rear VL6180X not found");
}

// An overflow status means no surface was found within range at all —
// exactly what happens when a sensor looks out over a cliff/table edge —
// so it's mapped to a large distance rather than ignored like other
// (noise/convergence) error statuses, which just keep the last good value.
static uint16_t applyRangeStatus(uint8_t range, uint8_t status, uint16_t lastValue)
{
    if (status == VL6180X_ERROR_NONE) return range;
    if (status == VL6180X_ERROR_RAWOFLOW || status == VL6180X_ERROR_RANGEOFLOW) return 255;
    return lastValue;
}

void ToF::update()
{
    // Sensors free-run in continuous mode; we just poll for a finished
    // reading each frame rather than blocking on readRange().
    if (_frontOk)
    {
        PCA.select(PCA_VL_FRONT);
        if (_frontSensor.isRangeComplete())
        {
            uint8_t range = _frontSensor.readRangeResult();
            _front = applyRangeStatus(range, _frontSensor.readRangeStatus(), _front);
        }
    }

    if (_rearOk)
    {
        PCA.select(PCA_VL_REAR);
        if (_rearSensor.isRangeComplete())
        {
            uint8_t range = _rearSensor.readRangeResult();
            _rear = applyRangeStatus(range, _rearSensor.readRangeStatus(), _rear);
        }
    }
}

uint16_t ToF::front()
{
    return _front;
}

uint16_t ToF::rear()
{
    return _rear;
}
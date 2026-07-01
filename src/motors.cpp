#include "motors.h"
#include "config.h"

#define LED_BLINK_MS 200

// L298N pins
#define LF 0
#define LB 1
#define RF 2
#define RB 3

// PWM channels
#define CH_LF 0
#define CH_LB 1
#define CH_RF 2
#define CH_RB 3

#define PWM_FREQ 20000
#define PWM_RES 8

void Motors::begin()
{
    ledcSetup(CH_LF, PWM_FREQ, PWM_RES);
    ledcSetup(CH_LB, PWM_FREQ, PWM_RES);
    ledcSetup(CH_RF, PWM_FREQ, PWM_RES);
    ledcSetup(CH_RB, PWM_FREQ, PWM_RES);

    ledcAttachPin(LF, CH_LF);
    ledcAttachPin(LB, CH_LB);
    ledcAttachPin(RF, CH_RF);
    ledcAttachPin(RB, CH_RB);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    stop();
}

void Motors::setSpeed(int left, int right)
{
    _left = constrain(left, -255, 255);
    _right = constrain(right, -255, 255);
}

void Motors::applyMotor(int fwdPin, int backPin, int speed)
{
    int pwmF = 0;
    int pwmB = 0;

    if (speed > 0) pwmF = speed;
    else pwmB = -speed;

    ledcWrite(fwdPin, pwmF);
    ledcWrite(backPin, pwmB);
}

void Motors::update()
{
    applyMotor(CH_LF, CH_LB, _left);
    applyMotor(CH_RF, CH_RB, _right);

    if (_left != 0 || _right != 0)
    {
        if (millis() - _ledTimer >= LED_BLINK_MS)
        {
            _ledTimer = millis();
            _ledState = !_ledState;
            digitalWrite(LED_PIN, _ledState);
        }
    }
    else
    {
        _ledState = false;
        _ledTimer = 0;
        digitalWrite(LED_PIN, LOW);
    }
}

void Motors::stop()
{
    _left = 0;
    _right = 0;

    ledcWrite(CH_LF, 0);
    ledcWrite(CH_LB, 0);
    ledcWrite(CH_RF, 0);
    ledcWrite(CH_RB, 0);
}
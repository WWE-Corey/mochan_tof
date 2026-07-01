#include "pca9548a.h"
#include "config.h"
#include <Wire.h>

void PCA9548A::begin()
{
    Wire.beginTransmission(PCA_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();
}

void PCA9548A::select(uint8_t channel)
{
    if (channel > 7) return;

    Wire.beginTransmission(PCA_ADDR);
    Wire.write(1 << channel);
    Wire.endTransmission();
}
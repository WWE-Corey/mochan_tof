#include "eyes.h"
#include "pca9548a.h"
#include <config.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

extern PCA9548A PCA;

Adafruit_SSD1306 display(128, 64, &Wire, -1);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

void Eyes::begin()
{
    PCA.select(PCA_OLED);
    delay(5);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("Eyes: OLED not found, continuing without display");
        return;
    }

    display.clearDisplay();
    display.display();

    delay(10);

    PCA.select(PCA_OLED);
    delay(2);

    roboEyes.begin(128, 64, 100);

    roboEyes.setAutoblinker(ON, 3, 2);
    roboEyes.setIdleMode(ON, 2, 2);
    roboEyes.setMood(DEFAULT);
}

void Eyes::setMood(EyeMood mood)
{
    PCA.select(PCA_OLED);

    switch (mood)
    {
        case MOOD_IDLE:
            roboEyes.setMood(DEFAULT);
            break;

        case MOOD_HAPPY:
            roboEyes.setMood(HAPPY);
            break;

        case MOOD_WARN:
            roboEyes.setMood(TIRED);
            break;

        case MOOD_DANGER:
            roboEyes.setMood(ANGRY);
            break;

        case MOOD_TIRED:
            roboEyes.setMood(TIRED);
            break;

        default:
            roboEyes.setMood(DEFAULT);
            break;
    }
}

void Eyes::confused()
{
    PCA.select(PCA_OLED);
    roboEyes.anim_confused();
}

void Eyes::toggleCyclops()
{
    PCA.select(PCA_OLED);
    roboEyes.setCyclops(!roboEyes.cyclops);
}

void Eyes::setCuriousLook(bool on)
{
    PCA.select(PCA_OLED);
    roboEyes.setCuriosity(on);
}

void Eyes::update()
{
    PCA.select(PCA_OLED);   // ALWAYS reassert channel
    roboEyes.update();
}
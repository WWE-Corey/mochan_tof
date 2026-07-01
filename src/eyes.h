#pragma once

enum EyeMood
{
    MOOD_IDLE,
    MOOD_HAPPY,
    MOOD_WARN,
    MOOD_DANGER,
    MOOD_TIRED
};

class Eyes {
public:
    void begin();
    void update();
    void setMood(EyeMood mood);
    void confused();           // one-shot "confused" shake animation
    void toggleCyclops();      // toggle single-eye rendering mode
    void setCuriousLook(bool on); // enlarge outer eye when looking left/right
};
/*
 * sound_engine.h
 * Non-blocking sound engine for MOCHAN robot.
 * Place this file in the same folder as mochan_robot.ino.
 * The Arduino IDE includes it before running its preprocessor,
 * so the Note struct is always visible when forward declarations
 * are injected.
 */

#pragma once
#include <Arduino.h>

// ── Note type ─────────────────────────────────────────────────────
struct Note {
  uint16_t freq;   // Hz — 0 = rest (noTone)
  uint16_t ms;     // duration in milliseconds
};

// ── Sequences (const, stored in flash) ───────────────────────────
#define MAX_NOTES 20

// R2-D2-style astromech chirps: fast, zigzagging pitch jumps and warbling
// glides instead of smooth tones, in the spirit of classic droid vocalizations.
static const Note SEQ_BEEP[]         = {
  {1800,35},{0,15},{3400,30},{0,15},{2200,35},{0,15},{4200,25},{0,15},{2800,40},{0,30}
};
static const Note SEQ_WHISTLE_UP[]   = {
  {1200,16},{1500,14},{1350,16},{1700,14},{1550,16},{1950,14},{1800,16},{2250,14},
  {2050,16},{2550,14},{2350,16},{2900,14},{2650,16},{3300,14},{3000,16},{3700,14},
  {3400,16},{4000,40},{0,150}
};
static const Note SEQ_WHISTLE_DOWN[] = {
  {3800,16},{3400,14},{3600,16},{3100,14},{3300,16},{2800,14},{3000,16},{2500,14},
  {2700,16},{2200,14},{2400,16},{1900,14},{2100,16},{1700,14},{1850,16},{1500,14},
  {1650,16},{1300,40},{0,150}
};
static const Note SEQ_ALERT[]        = {
  {3000,40},{1200,40},{3200,35},{1100,35},{3400,30},{1000,30},{3600,28},{900,28},
  {3800,25},{800,25},{0,80}
};
static const Note SEQ_HAPPY[]        = {
  {1800,50},{2200,40},{2000,35},{2600,40},{2400,35},{3000,40},{2800,35},{3600,45},
  {3300,35},{4200,60},{0,60}
};
static const Note SEQ_SAD[]          = {
  {1800,90},{1600,30},{1700,90},{0,30},{1400,100},{1250,30},{1350,100},{0,30},
  {1050,130},{950,40},{1000,130},{0,80},{750,200}
};

// ── Engine state ──────────────────────────────────────────────────
static Note          _soundSeq[MAX_NOTES];
static uint8_t       _soundLen     = 0;
static uint8_t       _soundIdx     = 0;
static unsigned long _noteStart    = 0;
static bool          _soundPlaying = false;
static bool          _soundEnabled = true;

// ── Engine functions ──────────────────────────────────────────────
// Resets on every boot (no flash/NVS write) — "persists" only for the
// current power cycle, by design.
inline bool soundEnabled() { return _soundEnabled; }

inline void soundSetEnabled(bool enabled) {
  _soundEnabled = enabled;
  if (!enabled) {
    noTone(PIN_BUZZER);
    _soundPlaying = false;
  }
}

inline void soundPlay(const Note* seq, uint8_t len) {
  if (!_soundEnabled) return;
  memcpy(_soundSeq, seq, len * sizeof(Note));
  _soundLen     = len;
  _soundIdx     = 0;
  _soundPlaying = true;
  _noteStart    = millis();
  if (seq[0].freq) tone(PIN_BUZZER, seq[0].freq);
  else             noTone(PIN_BUZZER);
}

inline void soundTick() {
  if (!_soundPlaying) return;
  if (millis() - _noteStart >= _soundSeq[_soundIdx].ms) {
    _soundIdx++;
    if (_soundIdx >= _soundLen) {
      noTone(PIN_BUZZER);
      _soundPlaying = false;
      return;
    }
    _noteStart = millis();
    if (_soundSeq[_soundIdx].freq) tone(PIN_BUZZER, _soundSeq[_soundIdx].freq);
    else                            noTone(PIN_BUZZER);
  }
}

inline bool soundBusy() { return _soundPlaying; }

// ── Named sound triggers ──────────────────────────────────────────
// Note counts are derived from each array's actual size so they can never
// drift out of sync and overrun the array in soundPlay()'s memcpy.
inline void sndBeep()        { soundPlay(SEQ_BEEP,         sizeof(SEQ_BEEP)         / sizeof(Note)); }
inline void sndWhistleUp()   { soundPlay(SEQ_WHISTLE_UP,   sizeof(SEQ_WHISTLE_UP)   / sizeof(Note)); }
inline void sndWhistleDown() { soundPlay(SEQ_WHISTLE_DOWN, sizeof(SEQ_WHISTLE_DOWN) / sizeof(Note)); }
inline void sndAlert()       { soundPlay(SEQ_ALERT,        sizeof(SEQ_ALERT)        / sizeof(Note)); }
inline void sndHappy()       { soundPlay(SEQ_HAPPY,        sizeof(SEQ_HAPPY)        / sizeof(Note)); }
inline void sndSad()         { soundPlay(SEQ_SAD,          sizeof(SEQ_SAD)          / sizeof(Note)); }
#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "pca9548a.h"
#include "eyes.h"
#include "tof.h"
#include "robot.h"
#include "motors.h"
#include "control.h"
#include "sound_engine.h"
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

PCA9548A PCA;
Eyes eyes;
ToF tof;
Robot robot;
Motors motors;
// ControlState control;

unsigned long lastFrame = 0;
const int FRAME_TIME_MS = 20; // 50Hz loop

void handleRoot();
void setupServer();

void setupServer() {
  server.on("/", handleRoot);

  server.on("/s", []() {
    control.mode = MODE_MANUAL;
    control.throttle = 0;
    control.steering = 0;
    sndAlert();
    server.send(200, "text/plain", "STOP");
  });

  server.on("/man", []() {
    control.mode = MODE_MANUAL;
    server.send(200, "text/plain", "MANUAL");
  });

  server.on("/auto", []() {
    control.mode = MODE_AUTO;
    server.send(200, "text/plain", "AUTO");
  });

  server.on("/curious", []() {
    control.mode = MODE_CURIOUS;
    server.send(200, "text/plain", "CURIOUS");
  });

  server.on("/soundon", []() {
    soundSetEnabled(true);
    server.send(200, "text/plain", "SOUND ON");
  });

  server.on("/soundoff", []() {
    soundSetEnabled(false);
    server.send(200, "text/plain", "SOUND OFF");
  });

  server.on("/drive", []() {
    if (!server.hasArg("t") || !server.hasArg("s")) {
      server.send(400, "text/plain", "missing args");
      return;
    }

    control.throttle = server.arg("t").toInt();
    control.steering = server.arg("s").toInt();

    server.send(200, "text/plain", "OK");
  });

  server.on("/mood", []() {
    if (!server.hasArg("m")) {
      server.send(400, "text/plain", "missing m");
      return;
    }

    String m = server.arg("m");

    // One-shot effects: not a sustained mood, so they bypass moodOverride
    // and trigger directly instead of being re-applied every frame.
    if (m == "confused") {
      eyes.confused();
      server.send(200, "text/plain", "OK");
      return;
    }
    if (m == "cyclops") {
      eyes.toggleCyclops();
      server.send(200, "text/plain", "OK");
      return;
    }

    EyeMood mood;
    if (m == "idle") mood = MOOD_IDLE;
    else if (m == "happy") mood = MOOD_HAPPY;
    else if (m == "tired") mood = MOOD_TIRED;
    else if (m == "danger") mood = MOOD_DANGER;
    else {
      server.send(400, "text/plain", "unknown mood");
      return;
    }

    control.moodOverride = mood;
    control.moodOverrideActive = true;
    server.send(200, "text/plain", "OK");
  });

  server.on("/sound", []() {
    if (!server.hasArg("n")) {
      server.send(400, "text/plain", "missing n");
      return;
    }

    String n = server.arg("n");
    if (n == "beep") sndBeep();
    else if (n == "whistleup") sndWhistleUp();
    else if (n == "whistledown") sndWhistleDown();
    else if (n == "alert") sndAlert();
    else if (n == "happy") sndHappy();
    else if (n == "sad") sndSad();
    else {
      server.send(400, "text/plain", "unknown sound");
      return;
    }

    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

void handleRoot() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body {
  margin:0;
  min-height:100vh;
  background:#0b0f14;
  color:#00ffe1;
  font-family:Arial;
  display:flex;
  justify-content:center;
  align-items:center;
  user-select:none;
}

.panel {
  width:320px;
  padding:20px;
  border-radius:18px;
  background:rgba(0,255,225,0.05);
  border:1px solid rgba(0,255,225,0.3);
  box-shadow:0 0 20px rgba(0,255,225,0.2);
  text-align:center;
}

h2 { margin-top:0; }
h4 { margin:14px 0 6px; }

button {
  margin:5px;
  padding:10px 15px;
  border:none;
  border-radius:10px;
  background:linear-gradient(145deg,#00ffe1,#00b3a4);
  font-weight:bold;
  font-size:16px;
}

.stop {
  background:linear-gradient(145deg,#ff5555,#aa0000);
  color:white;
}

.row { display:flex; flex-wrap:wrap; justify-content:center; }

.dpad {
  display:grid;
  grid-template-areas:
    ".    up    ."
    "left stop  right"
    ".    down  .";
  grid-template-columns:64px 64px 64px;
  grid-template-rows:64px 64px 64px;
  gap:8px;
  justify-content:center;
  margin:15px auto;
}
.dkey { margin:0; font-size:22px; }
#btnUp    { grid-area:up; }
#btnDown  { grid-area:down; }
#btnLeft  { grid-area:left; }
#btnRight { grid-area:right; }
#btnStop  { grid-area:stop; }
</style>
</head>

<body>

<div class="panel" id="homeScreen">
  <h2>Mochan Control</h2>
  <button onclick="goAuto()">AUTO</button>
  <button onclick="goManual()">MANUAL</button>
  <button onclick="goCurious()">CURIOUS</button>
  <br>
  <button id="btnSound" onclick="toggleSound()">Sound: ...</button>
</div>

<div class="panel" id="manualScreen" style="display:none">
  <h2>Manual Mode</h2>

  <div class="dpad">
    <button class="dkey" id="btnUp">&#9650;</button>
    <button class="dkey" id="btnLeft">&#9664;</button>
    <button class="dkey stop" id="btnStop">STOP</button>
    <button class="dkey" id="btnRight">&#9654;</button>
    <button class="dkey" id="btnDown">&#9660;</button>
  </div>

  <h4>Mood</h4>
  <div class="row">
    <button onclick="setMood('idle')">Idle</button>
    <button onclick="setMood('happy')">Happy</button>
    <button onclick="setMood('confused')">Confused</button>
    <button onclick="setMood('danger')">Danger</button>
    <button onclick="setMood('tired')">Tired</button>
    <button onclick="setMood('cyclops')">Cyclops</button>
  </div>

  <h4>Sounds</h4>
  <div class="row">
    <button onclick="playSound('beep')">Beep</button>
    <button onclick="playSound('whistleup')">Whistle Up</button>
    <button onclick="playSound('whistledown')">Whistle Down</button>
    <button onclick="playSound('alert')">Alert</button>
    <button onclick="playSound('happy')">Happy</button>
    <button onclick="playSound('sad')">Sad</button>
  </div>

  <br>
  <button onclick="goAuto()">Back</button>
</div>

<script>
function showHome(){
  document.getElementById('homeScreen').style.display = 'block';
  document.getElementById('manualScreen').style.display = 'none';
}
function showManual(){
  document.getElementById('homeScreen').style.display = 'none';
  document.getElementById('manualScreen').style.display = 'block';
}

function goAuto(){ fetch('/auto'); showHome(); }
function goManual(){ fetch('/man'); showManual(); }
function goCurious(){ fetch('/curious'); showHome(); }

let soundOn = __SOUND_ENABLED__;
function renderSoundButton(){
  document.getElementById('btnSound').textContent = 'Sound: ' + (soundOn ? 'ON' : 'OFF');
}
function toggleSound(){
  soundOn = !soundOn;
  fetch(soundOn ? '/soundon' : '/soundoff');
  renderSoundButton();
}
renderSoundButton();

function drive(t, s){ fetch(`/drive?t=${t}&s=${s}`); }
function stopDrive(){ fetch('/drive?t=0&s=0'); }

function setMood(m){ fetch('/mood?m=' + m); }
function playSound(n){ fetch('/sound?n=' + n); }

function bindHold(el, t, s){
  const press   = (e) => { e.preventDefault(); drive(t, s); };
  const release = (e) => { e.preventDefault(); stopDrive(); };
  el.addEventListener('mousedown',  press);
  el.addEventListener('touchstart', press);
  el.addEventListener('mouseup',    release);
  el.addEventListener('mouseleave', release);
  el.addEventListener('touchend',   release);
  el.addEventListener('touchcancel',release);
}

bindHold(document.getElementById('btnUp'),    200, 0);
bindHold(document.getElementById('btnDown'), -200, 0);
bindHold(document.getElementById('btnLeft'),    0, -200);
bindHold(document.getElementById('btnRight'),   0, 200);
document.getElementById('btnStop').addEventListener('click', () => fetch('/s'));

const dirKeys = {
  ArrowUp:    [200, 0],
  ArrowDown:  [-200, 0],
  ArrowLeft:  [0, -200],
  ArrowRight: [0, 200]
};
document.addEventListener('keydown', (e) => {
  if (e.repeat || !dirKeys[e.key]) return;
  drive(...dirKeys[e.key]);
});
document.addEventListener('keyup', (e) => {
  if (!dirKeys[e.key]) return;
  stopDrive();
});
</script>

</body>
</html>
)rawliteral";

  page.replace("__SOUND_ENABLED__", soundEnabled() ? "true" : "false");

  server.send(200, "text/html", page);
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n--- Mochan booting ---");

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);

    PCA.begin();
    Serial.println("PCA9548A ready");

    eyes.begin();
    Serial.println("Eyes ready");

    tof.begin();
    Serial.println("ToF ready");

    motors.begin();
    Serial.println("Motors ready");

    // tone()/noTone() default to LEDC channel 0, which Motors also uses
    // (CH_LF). Move the buzzer to an unused channel so playing a sound
    // doesn't steal the left-forward motor's PWM output mid-drive.
    setToneChannel(5);

    robot.begin(&tof, &eyes, &motors);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    setupServer();
    Serial.println("Web server ready");
}

void loop()
{
    server.handleClient();
    soundTick();

    if (millis() - lastFrame < FRAME_TIME_MS)
        return;

    lastFrame = millis();

    // -------------------------
    // 1. SENSOR PHASE
    // -------------------------
    tof.update();

    // -------------------------
    // 2. BRAIN PHASE
    // -------------------------
    robot.update();

    // -------------------------
    // 3. MOTOR PHASE
    // -------------------------
    motors.update();

    // -------------------------
    // 4. UI PHASE (OLED LAST)
    // -------------------------
    eyes.update();

    // -------------------------
    // 5. SOUND PHASE
    // -------------------------
    static bool wasTurning = false;
    bool isTurning = abs(state.steer) >= 20;
    if (isTurning && !wasTurning) {
        if (random(2) == 0) sndBeep();
        else if (random(2) == 0) sndWhistleUp();
        else sndWhistleDown();
    }
    wasTurning = isTurning;

    static bool wasCliff = false;
    bool isCliffNow = state.cliffFront || state.cliffRear;
    if (isCliffNow && !wasCliff) sndSad();
    wasCliff = isCliffNow;

    static unsigned long nextRandomSound = millis() + random(5000, 15000);
    if (!soundBusy() && millis() >= nextRandomSound) {
        nextRandomSound = millis() + random(5000, 15000);
        switch (random(3)) {
            case 0: sndBeep(); break;
            case 1: if (random(2) == 0) sndWhistleUp(); else sndWhistleDown(); break;
            default: sndHappy(); break;
        }
    }
}
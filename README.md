# Mochan with ToF

An ESP32-C3 based autonomous robot with OLED eyes, Time-of-Flight cliff detection, WiFi web control, and R2D2-style sounds.

---

## Features

- **Auto mode** — random wandering (forward, arc turns, in-place pivots, reversing) with thinking pauses between movements and real-time cliff/edge detection via VL6180X sensors
- **Manual mode** — web UI D-pad with hold-to-drive controls, keyboard arrow key support
- **Curious mode** — stays in place, performs partial in-place turns to look around
- **OLED eyes** — RoboEyes expressions that react to robot state (Idle, Happy, Warn, Danger, Tired, Confused, Cyclops)
- **Sound engine** — non-blocking R2D2-style chirps triggered by turning, cliff detection, stop commands, and random ambient playback; global Sound ON/OFF toggle
- **Activity LED** — blinks on GPIO 10 whenever motors are running
- **WiFi control** — robot creates its own access point; control panel served at `192.168.4.1`

---

## Hardware

| Component | Description |
|---|---|
| ESP32-C3 DevKitC-02 | Main microcontroller |
| PCA9548A | 8-channel I2C multiplexer |
| SSD1306 128×64 | OLED display (robot eyes) |
| VL6180X × 2 | Time-of-Flight sensors (cliff detection) |
| L298N | Dual H-bridge motor driver |
| 2× DC motor | Drive wheels |
| 14250 Li-ion | Primary battery (3.7V, ~600mAh) |
| TP4056 module | Li-ion battery charger with protection |
| MT3608 module | Boost converter (3.7V → 5V) |
| Slide switch | System power on/off |
| Passive buzzer | Sound output |
| LED + 330Ω resistor | Motor activity indicator |

---

## Wiring Schematic

### Power

```
14250 battery
    (+) ──► TP4056 B+
    (−) ──► TP4056 B−

TP4056 OUT+ ──► Slide switch pin 1
              Slide switch pin 2 ──► MT3608 IN+
                                     MT3608 IN− ──► GND rail
                                     MT3608 OUT+ (set to 5.0V) ──► 5V rail
                                     MT3608 OUT− ──► GND rail

5V rail ──► ESP32-C3 5V pin
        ──► L298N VS  (logic supply)
        ──► L298N VSS (motor supply)

TP4056 OUT− ──► GND rail
ESP32-C3 GND ──► GND rail
ESP32-C3 3.3V pin ──► 3.3V rail  (powers sensors & peripherals via onboard LDO)
```

> **Note:** Set the MT3608 trim pot to exactly 5.0V before connecting any components.

---

### ESP32-C3 DevKitC-02

| GPIO | Function | Connects to |
|---|---|---|
| GPIO 8 | I2C SDA | PCA9548A SDA |
| GPIO 9 | I2C SCL | PCA9548A SCL |
| GPIO 0 | Motor LF — LEDC ch 0 | L298N IN1 |
| GPIO 1 | Motor LB — LEDC ch 1 | L298N IN2 |
| GPIO 2 | Motor RF — LEDC ch 2 | L298N IN3 |
| GPIO 3 | Motor RB — LEDC ch 3 | L298N IN4 |
| GPIO 10 | Activity LED | LED → 330Ω → GND |
| GPIO 20 | Buzzer | Passive buzzer → GND |
| 5V pin | Power input | 5V rail |
| 3.3V pin | Power output | 3.3V rail |
| GND | Ground | GND rail |

---

### PCA9548A I2C Multiplexer (address 0x70)

| Pin | Connects to |
|---|---|
| SDA | ESP32 GPIO 8 |
| SCL | ESP32 GPIO 9 |
| VCC | 3.3V rail |
| GND | GND rail |
| Channel 2 SDA/SCL | SSD1306 OLED |
| Channel 3 SDA/SCL | Front VL6180X |
| Channel 4 SDA/SCL | Rear VL6180X |

---

### SSD1306 OLED (address 0x3C)

| Pin | Connects to |
|---|---|
| SDA | PCA9548A Channel 2 SDA |
| SCL | PCA9548A Channel 2 SCL |
| VCC | 3.3V rail |
| GND | GND rail |

---

### VL6180X × 2 (address 0x29 each, isolated by mux)

| Pin | Front sensor | Rear sensor |
|---|---|---|
| SDA | PCA9548A Ch 3 | PCA9548A Ch 4 |
| SCL | PCA9548A Ch 3 | PCA9548A Ch 4 |
| VCC | 3.3V rail | 3.3V rail |
| GND | GND rail | GND rail |

> Both sensors share address 0x29 — the PCA9548A selects only one channel at a time so there is no address conflict.

---

### L298N Motor Driver

| Pin | Connects to | Notes |
|---|---|---|
| IN1 | ESP32 GPIO 0 | Left motor forward PWM |
| IN2 | ESP32 GPIO 1 | Left motor reverse PWM |
| IN3 | ESP32 GPIO 2 | Right motor forward PWM |
| IN4 | ESP32 GPIO 3 | Right motor reverse PWM |
| ENA | VCC (jumpered) | Always enabled; speed via IN1/IN2 PWM |
| ENB | VCC (jumpered) | Always enabled; speed via IN3/IN4 PWM |
| OUT1 / OUT2 | Left motor terminals | |
| OUT3 / OUT4 | Right motor terminals | |
| VS | 5V rail | Logic supply |
| VSS | 5V rail | Motor supply |
| GND | GND rail | |

---

### Passive Components

| Component | Pin 1 | Pin 2 | Notes |
|---|---|---|---|
| Activity LED | ESP32 GPIO 10 | GND via 330Ω resistor | Blinks during motor activity |
| Passive buzzer | ESP32 GPIO 20 | GND | LEDC channel 5 |

---

## Firmware Setup

### Dependencies (platformio.ini)

```
lib_deps =
    adafruit/Adafruit SSD1306
    adafruit/Adafruit GFX Library
    https://github.com/adafruit/Adafruit_VL6180X
    fluxgarage/FluxGarage RoboEyes@^1.1.1
```

### Build flags

```
build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
```

Required for serial output over the ESP32-C3's native USB-Serial-JTAG port. Without these, `Serial.print()` writes to UART0 pins which are not connected to the USB cable.

### WiFi

The robot creates its own access point on boot:

| Setting | Value |
|---|---|
| SSID | `Mochan` |
| Password | `mochanbot` |
| Control panel | `http://192.168.4.1` |

To change these, edit `src/config.h`:

```cpp
#define WIFI_AP_SSID     "Mochan"
#define WIFI_AP_PASSWORD "mochanbot"
```

---

## Web API

| Endpoint | Action |
|---|---|
| `GET /` | Serve control panel HTML |
| `GET /auto` | Switch to Auto mode |
| `GET /man` | Switch to Manual mode |
| `GET /curious` | Switch to Curious mode |
| `GET /drive?t=&s=` | Set throttle (−255 to 255) and steering (−255 to 255) |
| `GET /s` | Stop motors + play Alert sound |
| `GET /mood?m=` | Set eye mood: `idle`, `happy`, `tired`, `danger`, `confused`, `cyclops` |
| `GET /sound?n=` | Play sound: `beep`, `whistleup`, `whistledown`, `alert`, `happy`, `sad` |
| `GET /soundon` | Enable sound |
| `GET /soundoff` | Disable sound (persists until power cycle) |

---

## Pin Summary

| GPIO | Assignment |
|---|---|
| 0 | L298N IN1 (left motor forward) |
| 1 | L298N IN2 (left motor reverse) |
| 2 | L298N IN3 (right motor forward) |
| 3 | L298N IN4 (right motor reverse) |
| 8 | I2C SDA |
| 9 | I2C SCL |
| 10 | Activity LED |
| 20 | Passive buzzer |

---

## Crude Wiring Diagram

<img width="816" height="1056" alt="Mochan_ToF_wires" src="https://github.com/user-attachments/assets/0fd364b9-283a-468e-8682-4cafea041cab" />

---

## Resources

### Contributors  
- [Original Project](https://www.huyvector.org/robots-kinetic/diy-cute-desk-robot-mo-chan) - Credit to Huy Vector for the original (to me at least) Mochan Desk Robot.

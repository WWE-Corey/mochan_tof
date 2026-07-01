#pragma once
#include <Arduino.h>

// I2C
#define SDA_PIN 8
#define SCL_PIN 9

// PCA9548A
#define PCA_ADDR 0x70
#define PCA_OLED 2
#define PCA_VL_FRONT 3
#define PCA_VL_REAR 4

// IO
#define LED_PIN    10
#define BUZZER_PIN 20
#define PIN_BUZZER BUZZER_PIN

// WiFi (robot acts as its own access point)
#define WIFI_AP_SSID "Mochan"
#define WIFI_AP_PASSWORD "mochanbot"
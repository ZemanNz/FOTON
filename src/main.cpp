#include "SmartServoBus.hpp"
#include "RBCX.h"
#include <Arduino.h>
#include <thread>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>


auto &man = rb::Manager::get();

#define XSHUT1 13
#define XSHUT2 33

Adafruit_VL53L0X laser1;
Adafruit_VL53L0X laser2;



void setup() {
    Serial.begin(115200);
    man.install();

    // Nastavíme XSHUT piny jako výstup a deaktivujeme oba senzory
    pinMode(XSHUT1, OUTPUT);
    pinMode(XSHUT2, OUTPUT);
    digitalWrite(XSHUT1, LOW);
    digitalWrite(XSHUT2, LOW);

    delay(10); // krátká pauza

    // Spuštění I2C sběrnice
    Wire.begin(22, 21);
    Wire.setClock(400000);

    // Aktivuj první senzor
    digitalWrite(XSHUT1, HIGH);
    delay(10);
    if (!laser1.begin(0x30)) {  // změna výchozí adresy z 0x29 → 0x30
        Serial.println("Nepodařilo se inicializovat senzor 1!");
        while (1);
    }

    // Aktivuj druhý senzor
    digitalWrite(XSHUT2, HIGH);
    delay(10);
    if (!laser2.begin(0x31)) {  // změna výchozí adresy z 0x29 → 0x31
        Serial.println("Nepodařilo se inicializovat senzor 2!");
        while (1);
    }

    Serial.println("Oba senzory připraveny.");
}

void loop() {
    VL53L0X_RangingMeasurementData_t m1, m2;

    laser1.rangingTest(&m1, false);
    laser2.rangingTest(&m2, false);

    Serial.print("Senzor 1: ");
    if (m1.RangeStatus != 4) {
        Serial.print(m1.RangeMilliMeter);
        Serial.print(" mm");
    } else {
        Serial.print("Mimo rozsah");
    }

    Serial.print("  |  Senzor 2: ");
    if (m2.RangeStatus != 4) {
        Serial.print(m2.RangeMilliMeter);
        Serial.println(" mm");
    } else {
        Serial.println("Mimo rozsah");
    }

    delay(100);
}

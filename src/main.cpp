#include <Arduino.h>
#include <RBCX.h>
#include "gyro.hpp"

auto &man = rb::Manager::get();

void setup() {
    Serial.begin(115200);
    delay(500);

    man.install();

    // Inicializace a kalibrace gyroskopu (robot musí být v klidu)
    gyroInit();
}

void loop() {
    // Vypisování relativních úhlů pro osy X, Y, Z v reálném čase
    Serial.printf("Gyro angles [deg]: X=%.2f, Y=%.2f, Z=%.2f\n",
                  gyroGetAngleX(), gyroGetAngleY(), gyroGetAngleZ());

    // Resetování úhlů pomocí tlačítek na desce robota
    if (man.buttons().left() == 1) {
        gyroResetX();
        Serial.println("\n>>> Reseting X axis! <<<\n");
        delay(300);
    }
    if (man.buttons().right() == 1) {
        gyroResetY();
        Serial.println("\n>>> Reseting Y axis! <<<\n");
        delay(300);
    }
    if (man.buttons().up() == 1) {
        gyroResetZ();
        Serial.println("\n>>> Reseting Z axis (Heading)! <<<\n");
        delay(300);
    }
    if (man.buttons().down() == 1) {
        gyroResetAll();
        Serial.println("\n>>> Reseting ALL axes! <<<\n");
        delay(300);
    }

    delay(200);
}

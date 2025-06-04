#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <RBCX.h>

// Instance senzorů
Adafruit_VL53L0X laser1; // Přes Wire (21, 22)
Adafruit_VL53L0X laser2; // Přes Wire1 (26, 14)

// Vytvoření nové sběrnice Wire1
extern TwoWire Wire1;

void setup() {
  Serial.begin(115200);
  delay(100);

  // Inicializace první sběrnice a senzoru
  Wire.begin(21, 22);  
  //Wire.setClock(400000);
  if (!laser1.begin(0x29, false, &Wire)) {
    Serial.println("Nepodařilo se inicializovat senzor 1 na Wire!");
    while (1);
  }

  // Inicializace druhé sběrnice a senzoru
//   Wire1.begin(26, 14);
//   Wire1.setClock(400000);
//   if (!laser2.begin(0x29, false, &Wire1)) {
//     Serial.println("Nepodařilo se inicializovat senzor 2 na Wire1!");
//     while (1);
//   }

  Serial.println("Oba senzory připraveny.");
}

void loop() {
  VL53L0X_RangingMeasurementData_t m1, m2;

  // Provést měření
  laser1.rangingTest(&m1, false);
  //laser2.rangingTest(&m2, false);

  //Výpis výsledků
  Serial.print("Senzor 1 (Wire): ");
  if (m1.RangeStatus != 4) {
    Serial.print(m1.RangeMilliMeter);
    Serial.print(" mm");
  } else {
    Serial.print("Mimo rozsah");
  }
//delay(100); // Krátká prodleva mezi měřeními
//   Serial.print("  |  Senzor 2 (Wire1): ");
//   if (m2.RangeStatus != 4) {
//     Serial.print(m2.RangeMilliMeter);
//     Serial.println(" mm");
//   } else {
//     Serial.println("Mimo rozsah");
//   }
  delay(1000);
}

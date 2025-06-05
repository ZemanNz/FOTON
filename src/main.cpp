#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <RBCX.h>

// Instance senzorů
Adafruit_VL53L0X laser1; // Přes Wire (21, 22)
Adafruit_VL53L0X laser2; // Přes Wire1 (26, 14)

// Vytvoření nové sběrnice Wire1
extern TwoWire Wire1;

void WaitForStart()
{
    while (true)
    {
        if (man.buttons().on() == 1)
        {
            break;
        }
        delay(10);
    }
}

void CheckBattery()
{
    const auto &bat = man.battery();
    static const uint32_t VOLTAGE_MAX = 7200; //%edit
    static const uint32_t VOLTAGE_MIN = 6800; //%edit
    int i = 0;
    int voltage = 0;
    for (i = 0; i < 2; ++i)
    {
        voltage = bat.voltageMv();
        // Vypočítej procenta (omez na 0-100)
        int pct = (voltage - VOLTAGE_MIN) * 100 / (VOLTAGE_MAX - VOLTAGE_MIN);
        if (pct > 100)
            pct = 100;
        if (pct < 0)
            pct = 0;

        if (i > 0)
        {
            printf("Battery at %d%%, %dmv\n", pct, voltage);
            if (voltage < VOLTAGE_MIN)
            {
                printf("Je třeba nabít baterii!\n");
            }
        }
        delay(1000);
    }
}

void setup() {

   auto &man = rb::Manager::get(); // get manager instance as singleton
    man.install();  // install manager, this will initialize the hardware and the
    
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
  Wire1.begin(26, 14);
  Wire1.setClock(400000);
  if (!laser2.begin(0x29, false, &Wire1)) {
    Serial.println("Nepodařilo se inicializovat senzor 2 na Wire1!");
    while (1);
  }

  Serial.println("Oba senzory připraveny.");

  WaitForStart(); // Čekej na stisk tlačítka "ON" pro spuštění

  CheckBattery(); // Zkontroluj stav baterie

  
}

void loop() {
  VL53L0X_RangingMeasurementData_t m1, m2;

  // Provést měření
  laser1.rangingTest(&m1, false);
  laser2.rangingTest(&m2, false);

  //Výpis výsledků
  Serial.print("Senzor 1 (Wire): ");
  if (m1.RangeStatus != 4) {
    Serial.print(m1.RangeMilliMeter);
    Serial.print(" mm");
  } else {
    Serial.print("Mimo rozsah");
  }
//delay(100); // Krátká prodleva mezi měřeními
  Serial.print("  |  Senzor 2 (Wire1): ");
  if (m2.RangeStatus != 4) {
    Serial.print(m2.RangeMilliMeter);
    Serial.println(" mm");
  } else {
    Serial.println("Mimo rozsah");
  }
  delay(1000);
}

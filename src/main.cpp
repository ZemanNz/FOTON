#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <RBCX.h>

auto &man = rb::Manager::get(); // pro fungovani RBCX
#include "Grabber.hpp"
#include "Comunication.hpp"
#include "Movement.hpp"
#include "Arm.hpp"
#include "Sensors.hpp"


Grabber grab;
Movement move;
Communication comm;
Sensors sens;
Arm arm;

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
    static const uint32_t VOLTAGE_MAX = 7800; //%edit
    static const uint32_t VOLTAGE_MIN = 7000; //%edit
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


  sens.InitRGB(); // Inicializace RGB senzorů

  WaitForStart(); // Čekej na stisk tlačítka "ON" pro spuštění

  while (true)
  {
    sens.PrintRGBToSerial(); // Vytiskni barvy z RGB senzorů do sériového monitoru
    delay(1000); // Krátká prodleva pro stabil
  }
  
  servoBus.begin(2, UART_NUM_1, GPIO_NUM_27 ); // Inicializace sběrnice pro serva na GPIO 27 s UART1
  servoBus.set(0, 0_deg); 
  servoBus.set(1, 0_deg);  
  
  arm.SmallerUp();

  WaitForStart();
  servoBus.set(1, 28_deg);
  Serial.println("Servo 0 nastaveno na 30 stupňů");
  delay(1000); // Krátká prodleva pro stabilizaci
  WaitForStart();
  servoBus.set(1, 0_deg); 
  Serial.println("Servo 0 nastaveno na 0 stupňů");


  delay(1000); // Krátká prodleva pro stabilizaci
  WaitForStart();
  
  

  CheckBattery(); // Zkontroluj stav baterie

  // move.Straight(32000, 1000, 10000); // Příkaz pro pohyb robota rovně na 1 metr s rychlostí 2500 a timeoutem 5 sekund

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

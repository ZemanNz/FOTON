#include "SmartServoBus.hpp"
#include "RBCX.h"
#include <Arduino.h>
struct Communication
{
  int x_distance = 0;
  int y_distance = 0;

  // ceka nez dojde zprava z Raspberry Pi ze je pripraveno
  void WaitForReadyMessage()
  {
    while (true)
    {
      if (Serial.available() > 0)
      {
        String data = Serial.readStringUntil('\n');
        if (data == "ready")
        {
          man.leds().green(true);
          break;
        }
      }
      delay(10);
    }
  }

  // posle zpravu do Raspberry Pi ze je na pozici pro vyfoceni fotky
  void SendInPosstionMessage()
  {
    Serial.println("inposition");
  }

  void WaitForData()
  {
    while (true)
    {
      if (Serial.available() > 0)
      {
        int num = 0;
        String data = Serial.readStringUntil('\n');
        const char *daata = data.c_str();
        num = std::atoi(daata);

        if (num != 0)
        {
          man.leds().yellow(true);
          
        }
        delay(10);
        break;
      }
    }
  }

};

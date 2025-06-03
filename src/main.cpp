#include "SmartServoBus.hpp"
#include "RBCX.h"
#include <Arduino.h>
#include <thread>
auto &man = rb::Manager::get(); // pro fungovani RBCX
#include "Grabber.hpp"
#include "Comunication.hpp"
#include "Movement.hpp"

Grabber grab;
Movement move;
Communication comm;

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
    static const uint32_t VOLTAGE_MAX = 4200 * 2;
    static const uint32_t VOLTAGE_MIN = 7200;
    int voltage = 0;
    for (int i = 0; i < 2; ++i)
    {
        voltage = bat.voltageMv();
        int pct = (voltage - VOLTAGE_MIN) * 100 / (VOLTAGE_MAX - VOLTAGE_MIN);
        if (pct > 100)
            pct = 100;
        if (pct < 0)
            pct = 0;

        if (i > 0)
        { // První měření ignorovat
            printf("Battery at %d%%, %dmv\n", i, pct, voltage);
            if (voltage < VOLTAGE_MIN)
            {
                printf("Je třeba nabít baterii!\n");
            }
        }
        delay(1000);
    }
}

void setup()
{
    Serial.begin(115200);

    auto &man = rb::Manager::get(); // get manager instance as singleton
    man.install();                  // install manager

    servoBus.begin(2, UART_NUM_1, GPIO_NUM_27);
    servoBus.setAutoStop(0, false); // vypne autostop leveho serva
    servoBus.setAutoStop(1, false); // vypne autostop praveho serva

    // comm.WaitForData(); // cekani na zpravu z Raspberry

    CheckBattery();
    // WaitForStart();
    // man.leds().red(true);
    //  move.Straight(1000, 1000, 1000);
    //  move.BackwardUntillWall();
    // grab.Close();
    // delay(2000);
    // grab.Open();
    // delay(2000);
    // grab.Close();
}
void loop() {}
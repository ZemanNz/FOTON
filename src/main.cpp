#include "SmartServoBus.hpp"
#include "RBCX.h"
#include <Arduino.h>
#include <thread>
auto &man = rb::Manager::get(); // pro fungovani RBCX
#include "Grabber.hpp"
#include "Comunication.hpp"
#include "Movement.hpp"

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(10);
    } // wait for serial port to connect. Needed for native USB
}
void loop() {}
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

void setup()
{
    Serial.begin(115200);

    auto &man = rb::Manager::get(); // get manager instance as singleton
    man.install();                  // install manager

    const int ultrasoundId = 0; // ultrasound id
    // servoBus.begin(2, uart_port_t::UART_NUM_1, gpio_num_t::GPIO_NUM_4, 2560); // %edit
    // servoBus.setAutostop(2, false); // %edit

    while (true)
    {
    }
}
void loop() {}
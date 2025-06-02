#include "SmartServoBus.hpp"
#include "RBCX.h"
#include <Arduino.h>

// pro spravne fungovani SmartServoBus.hpp
using namespace lx16a;
static SmartServoBus servoBus;

// struktura grabber obsahuje vsechny funkce pro ovladani grabberu
struct Grabber
{
    // nastavy grabber na open pozici
    void Open()
    {
        servoBus.set(0, 240_deg); // prave servo
        servoBus.set(1, 0_deg);   // leve servo
    }
    // nastavy grabber na close pozici
    void Close()
    {
        servoBus.set(0, 220_deg);
        servoBus.set(1, 20_deg);
    }
};

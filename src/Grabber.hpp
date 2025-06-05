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
    void SmallerArmClose()
    {
        servoBus.set(0, 12_deg); 
    }
    void SmallerArmOpen()
    {
        servoBus.set(0, 0_deg); 
    }
    
    void BiggerArmClose()
    {
        servoBus.set(0, 28_deg); 
    }
    void BiggerArmOpen()
    {
        servoBus.set(0, 0_deg); 
    }
};

#include "SmartServoBus.hpp"
#include "RBCX.h"
#include<Arduino.h>

//pro spravne fungovani SmartServoBus.hpp
using namespace lx16a;
static SmartServoBus servoBus;



//struktura grabber obsahuje vsechny funkce pro ovladani grabberu
struct Grabber
{
    //funkce pro vypocet o jaky uhel se ma otocit prave servo
    Angle RightAngle(Angle angle)
    {
    angle = 240_deg - angle;
    return angle;
    }

    //mozne pozice grabberu
    enum grabber_state
        {
            closed,
            open,
            grab
        };

    grabber_state last_state = closed;//vychozi stav je zavreno
    //nastavy grabber na open pozici
    void Open(){
        if(last_state == closed){
            servoBus.set(0, 0_deg);
            delay(1000);//nutna prodleva, aby nedoslo ke kolizi klepet pri otvirani
            servoBus.set(1, RightAngle(0_deg));
        }
        else{//grab
            servoBus.set(0, 0_deg); 
            servoBus.set(1, RightAngle(0_deg)); 
        }
        last_state = open;
    }
    //nastavy grabber na close pozici
    void Close(){
        if(last_state == open){
            servoBus.set(1, RightAngle(140_deg)); 
            delay(1000);//nutna prodleva, aby nedoslo ke kolizi klepet pri otvirani
            servoBus.set(0, 145_deg); 
        }
        else{//grab
            servoBus.set(0, 50_deg); 
            delay(1000);
            servoBus.set(1, RightAngle(140_deg));
            delay(1000);//nutna prodleva, aby nedoslo ke kolizi klepet pri otvirani
            servoBus.set(0, 145_deg); 
        }
        last_state = closed;
    }
    //nastavy grabber na grab pozici
    void Grab(){
        if(last_state == open){
            servoBus.set(0, 80_deg);
            servoBus.set(1, RightAngle(75_deg)); 
        }
        else{//close
            servoBus.set(0, 80_deg); 
    delay(1000);//nutna prodleva, aby nedoslo ke kolizi klepet pri otvirani
    servoBus.set(1, RightAngle(75_deg)); 
        }
        last_state = grab;
    }
};



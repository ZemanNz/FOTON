#include "SmartServoBus.hpp"
#include "RBCX.h"
#include <Arduino.h>

// pro spravne fungovani SmartServoBus.hpp
using namespace lx16a;
static SmartServoBus servoBus;

// Konstanty pro úhly serv (ve stupních)
// Servo 0 = malé klepeto (Smaller arm)
#define SMALLER_OPEN_DEG   3
#define SMALLER_CLOSE_DEG  28

// Servo 1 = velké klepeto (Bigger arm)
#define BIGGER_OPEN_DEG    12
#define BIGGER_CLOSE_DEG   34

// Výchozí rychlost pohybu serv
#define SERVO_DEFAULT_SPEED 200

// struktura grabber obsahuje vsechny funkce pro ovladani grabberu
struct Grabber
{
    // Inicializace obou serv s limity a parametry AutoStop
    void Init()
    {
        // Servo 0 (malé klepeto): limity 0°-50°
        servoBus.setAutoStop(0, false);
        servoBus.limit(0, Angle::deg(0), Angle::deg(50));
        servoBus.setAutoStopParams(
            SmartServoBus::AutoStopParams{
                .max_diff_centideg = 200,
                .max_diff_readings = 3,
            });
        Serial.println("Smart Servo 0 (male klepeto) inicializovano (limity: 0-50)");

        // Servo 1 (velké klepeto): limity 0°-50°
        servoBus.setAutoStop(1, false);
        servoBus.limit(1, Angle::deg(0), Angle::deg(50));
        servoBus.setAutoStopParams(
            SmartServoBus::AutoStopParams{
                .max_diff_centideg = 200,
                .max_diff_readings = 3,
            });
        Serial.println("Smart Servo 1 (velke klepeto) inicializovano (limity: 0-50)");
    }

    // --- Malé klepeto (Servo 0) ---
    void SmallerArmClose()
    {
        servoBus.setAutoStop(0, true);
        servoBus.set(0, Angle::deg(SMALLER_CLOSE_DEG), SERVO_DEFAULT_SPEED);
        Serial.printf("Servo 0: SoftMove CLOSE na %d deg\n", SMALLER_CLOSE_DEG);
    }
    void SmallerArmOpen()
    {
        servoBus.setAutoStop(0, false);
        servoBus.set(0, Angle::deg(SMALLER_OPEN_DEG), SERVO_DEFAULT_SPEED);
        Serial.printf("Servo 0: Move OPEN na %d deg\n", SMALLER_OPEN_DEG);
    }

    // --- Velké klepeto (Servo 1) ---
    void BiggerArmClose()
    {
        servoBus.setAutoStop(1, true);
        servoBus.set(1, Angle::deg(BIGGER_CLOSE_DEG), SERVO_DEFAULT_SPEED);
        Serial.printf("Servo 1: SoftMove CLOSE na %d deg\n", BIGGER_CLOSE_DEG);
    }
    void BiggerArmOpen()
    {
        servoBus.setAutoStop(1, false);
        servoBus.set(1, Angle::deg(BIGGER_OPEN_DEG), SERVO_DEFAULT_SPEED);
        Serial.printf("Servo 1: Move OPEN na %d deg\n", BIGGER_OPEN_DEG);
    }

    // --- Pozice ---
    byte GetSmallerPos()
    {
        float angle = servoBus.pos(0).deg();
        if (angle < 0) return 0;
        if (angle > 240) return 240;
        Serial.printf("Pozice serva 0 (male): %.1f deg\n", angle);
        return (byte)angle;
    }
    byte GetBiggerPos()
    {
        float angle = servoBus.pos(1).deg();
        if (angle < 0) return 0;
        if (angle > 240) return 240;
        Serial.printf("Pozice serva 1 (velke): %.1f deg\n", angle);
        return (byte)angle;
    }
};

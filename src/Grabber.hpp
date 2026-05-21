#include "SmartServoBus.hpp"
#include "RBCX.h"
#include <Arduino.h>

// pro spravne fungovani SmartServoBus.hpp
using namespace lx16a;
static SmartServoBus servoBus;

// Konstanty pro úhly serv (ve stupních)
// Servo 0 = malé klepeto (Smaller arm)
#define SMALLER_OPEN_DEG   3
#define SMALLER_CLOSE_DEG  24

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
                .max_diff_centideg = 250,
                .max_diff_readings = 4,
            });
        Serial.println("Smart Servo 0 (male klepeto) inicializovano (limity: 0-50)");

        // Servo 1 (velké klepeto): limity 0°-50°
        servoBus.setAutoStop(1, false);
        servoBus.limit(1, Angle::deg(0), Angle::deg(50));
        servoBus.setAutoStopParams(
            SmartServoBus::AutoStopParams{
                .max_diff_centideg = 250,
                .max_diff_readings = 3,
            });
        Serial.println("Smart Servo 1 (velke klepeto) inicializovano (limity: 0-50)");
    }

    // --- Obecné smart servo funkce ---
    void SmartServoMove(int id, int angle, int speed = SERVO_DEFAULT_SPEED)
    {
        servoBus.setAutoStop(id, false);
        servoBus.set(id, Angle::deg(angle), speed);
        Serial.printf("Smart Servo %d move na %d stupňů rychlostí %d\n", id, angle, speed);
    }

    void SmartServoSoftMove(int id, int angle, int speed = SERVO_DEFAULT_SPEED)
    {
        servoBus.setAutoStop(id, true);
        servoBus.set(id, Angle::deg(angle), speed);
        Serial.printf("Smart Servo %d soft_move na %d stupňů rychlostí %d\n", id, angle, speed);
    }

    byte GetServoPos(int id)
    {
        float angle = servoBus.pos(id).deg();
        if (angle < 0) return 0;
        if (angle > 240) return 240;
        Serial.printf("Pozice serva na id: %d je: %d\n", id, (byte)angle);
        return (byte)angle;
    }

    bool je_kostka_v_klepete(int id = 1)
    {
        int close_deg = (id == 0) ? SMALLER_CLOSE_DEG : BIGGER_CLOSE_DEG;
        SmartServoSoftMove(id, close_deg, SERVO_DEFAULT_SPEED);
        // Počkáme na dokončení pohybu
        vTaskDelay(pdMS_TO_TICKS(900));
        
        byte pos = GetServoPos(id);
        Serial.printf("Klepeta pozice po dozavreni (id %d): %d (limit: %d)\n", id, pos, close_deg);
        
        // Pokud je pozice menší o více než 3-4 stupně od cílové, servo bylo zastaveno kostkou
        if(pos < (close_deg - 3)){
            return true;
        }
        return false;
    }

    // --- Malé klepeto (Servo 0) ---
    void SmallerArmClose()
    {
        SmartServoSoftMove(0, SMALLER_CLOSE_DEG, SERVO_DEFAULT_SPEED);
    }
    void SmallerArmOpen()
    {
        SmartServoMove(0, SMALLER_OPEN_DEG, SERVO_DEFAULT_SPEED);
    }

    // --- Velké klepeto (Servo 1) ---
    void BiggerArmClose()
    {
        SmartServoSoftMove(1, BIGGER_CLOSE_DEG, SERVO_DEFAULT_SPEED);;
    }
    void BiggerArmOpen()
    {
        SmartServoMove(1, BIGGER_OPEN_DEG, SERVO_DEFAULT_SPEED);
    }

    // --- Pozice ---
    byte GetSmallerPos()
    {
        return GetServoPos(0);
    }
    byte GetBiggerPos()
    {
        return GetServoPos(1);
    }
};

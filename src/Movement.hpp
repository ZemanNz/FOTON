#include "SmartServoBus.hpp"
#include "RBCX.h"
#include <Arduino.h>

// Forward declaration of emergency stop function defined in main.cpp
void checkEmergencyStop();

struct Movement
{
  rb::MotorId motorL = rb::MotorId::M4; // levý motor
  rb::MotorId motorR = rb::MotorId::M1; // pravý motor
  double mm_to_ticks = 0.098;           // konstanta pro prepocet tics enkoderu na mm
  int wheel_base = 165;                 // vzdalenost mezi koly robota v mm
  int last_ticks_MR = 0;                // pravy motor
  int last_ticks_ML = 0;                // levy motor

  /**
   * Fuknkce pro pohyb robota v oblouku doprava
   * @param angle: jaky uhel oblouku robot ujede
   * @param radius: polomer oblouku vnitrniho kola robota
   *
   * @return None
   */
  void ArcRight(int angle, int radius)
  {
    // Nastaveni pocitadla tics na 0 pro oba motory
    man.motor(motorL).setCurrentPosition(0);
    man.motor(motorR).setCurrentPosition(0);
    // auto &man = rb::Manager::get();
    double inner_lenght = (((2 * PI * radius) / 360) * angle) / mm_to_ticks;                  // vzdalenost, kterou musi vnitrni kolo v oblouku ujet v tics enkoderu
    double outer_lenght = (((2 * 3.14 * (radius + wheel_base)) / 360) * angle) / mm_to_ticks; // vzdalenost, kterou musi vnejsi kolo v oblouku ujet v tics enkoderu
    // Serial.println(inner_lenght);
    int outer_sped = 5200;                                         // rychlost vnejsiho kola
    int inner_speed = -(outer_sped * inner_lenght) / outer_lenght; // vypocet pro rychlost vnitrniho kola
    // Serial.println(inner_speed);
    int tics_M2 = 0;
    int tics_M3 = 0;
    // Serial.println(outer_lenght);
    while ((inner_lenght > tics_M3) && (outer_lenght > tics_M2))
    {
      checkEmergencyStop();
      man.motor(motorL).speed(outer_sped);
      man.motor(motorR).speed(inner_speed);
      man.motor(motorL).requestInfo([&tics_M2](rb::Motor &info)
                                    { tics_M2 = -info.position(); });
      man.motor(motorR).requestInfo([&tics_M3](rb::Motor &info)
                                    { tics_M3 = -info.position(); });
      delay(10);
    }
  }

  void ArcLeft(int angle, int radius)
  {
    man.motor(motorL).setCurrentPosition(0);
    man.motor(motorR).setCurrentPosition(0);
    auto &man = rb::Manager::get();
    double inner_lenght = (((2 * PI * radius) / 360) * angle) / mm_to_ticks;
    double outer_lenght = (((2 * 3.14 * (radius + wheel_base)) / 360) * angle) / mm_to_ticks;
    Serial.println(inner_lenght);
    int outer_sped = -5200;
    int inner_speed = -(outer_sped * inner_lenght) / outer_lenght;
    Serial.println(inner_speed);
    int tics_M2 = 0;
    int tics_M3 = 0;
    Serial.println(outer_lenght);
    while ((inner_lenght > tics_M2) && (outer_lenght > tics_M3))
    {
      checkEmergencyStop();
      man.motor(motorR).speed(outer_sped);
      man.motor(motorL).speed(inner_speed);
      man.motor(motorR).requestInfo([&tics_M3](rb::Motor &info)
                                    {
            //printf("M3: position:%d\n", info.position());
            tics_M3 = -info.position(); });
      man.motor(motorL).requestInfo([&tics_M2](rb::Motor &info)
                                    { tics_M2 = -info.position(); });
      delay(10);
    }
  }

  void Acceleration(int speed_from, int speed_to, int distance_mm)
  {
    Serial.println("Acceleration");
    man.motor(motorL).setCurrentPosition(0);
    man.motor(motorR).setCurrentPosition(0);
    double distance_ticks = distance_mm / mm_to_ticks;
    double acc_const = speed_to / distance_ticks;
    int ticks_ML = 0;
    int ticks_MR = 0;
    int error = 0;
    while ((ticks_ML < distance_ticks) && (ticks_MR < distance_ticks))
    {
      checkEmergencyStop();
      error = 10 * (ticks_ML - ticks_MR);
      man.motor(motorL).speed((acc_const * ticks_ML + speed_from - error));
      man.motor(motorR).speed(-(acc_const * ticks_MR + speed_from + error));
      man.motor(motorR).requestInfo([&ticks_MR](rb::Motor &info)
                                    {
            //printf("M3: position:%d\n", info.position());
            ticks_MR = -info.position(); });
      man.motor(motorL).requestInfo([&ticks_ML](rb::Motor &info)
                                    {
            //printf("M2: position:%d\n", info.position());
            ticks_ML = info.position(); });

      delay(10);
    }
  }

  void TurnLeft(int angle)
  {
    man.motor(motorL).setCurrentPosition(0);
    man.motor(motorR).setCurrentPosition(0);
    int ticks_ML = 0;
    int ticks_MR = 0;
    int distance = ((PI * wheel_base) / 360) * angle / mm_to_ticks;
    while (distance > ticks_MR)
    {
      checkEmergencyStop();
      man.motor(motorL).speed(-1500);
      man.motor(motorR).speed(-1500);
      man.motor(motorR).requestInfo([&ticks_MR](rb::Motor &info)
                                    {
            //printf("M3: position:%d\n", info.position());
            ticks_MR = -info.position(); });
      man.motor(motorL).requestInfo([&ticks_ML](rb::Motor &info)
                                    {
            //printf("M2: position:%d\n", info.position());
            ticks_ML = info.position(); });

      delay(10);
    }
    man.motor(motorR).speed(0);
    man.motor(motorL).speed(0);
  }

  void TurnRight(int angle)
  {
    man.motor(motorL).setCurrentPosition(0);
    man.motor(motorR).setCurrentPosition(0);
    int ticks_ML = 0;
    int ticks_MR = 0;
    int distance = ((PI * wheel_base) / 360) * angle / mm_to_ticks;
    while (distance > ticks_MR)
    {
      checkEmergencyStop();
      man.motor(motorL).speed(1500);
      man.motor(motorR).speed(1500);
      man.motor(motorR).requestInfo([&ticks_MR](rb::Motor &info)
                                    {
            //Serial.println( -info.position());//"M3: position:%d\n",
            ticks_MR = info.position(); });
      man.motor(motorL).requestInfo([&ticks_ML](rb::Motor &info)
                                    {
            //printf("M2: position:%d\n", info.position());
            ticks_ML = -info.position(); });

      delay(10);
    }
    man.motor(motorR).speed(0);
    man.motor(motorL).speed(0);
  }

  void BackwardUntillWall(unsigned long timeout = 10000)
  {
    int start_timer = millis();
    while ((man.buttons().left() == 0 || man.buttons().right() == 0) && (millis() - start_timer < timeout)) //left je opravdu leve tlaitko
    { //(ticks_ML < distance)&& (ticks_MR < distance)
      checkEmergencyStop();
      if(man.buttons().left() == 1){
        man.motor(motorL).speed(-3000);
        man.motor(motorR).speed(1000);
      }
      else if(man.buttons().right() == 1){
        man.motor(motorR).speed(3000);
        man.motor(motorL).speed(-1000);  
      }
      else{
      man.motor(motorR).speed(2500);
      man.motor(motorL).speed(-2500);
      }
      delay(10);
    }
  man.motor(motorR).speed(0);
  man.motor(motorL).speed(0);
  }

  /**
   * Funkce pro pohyb robota rovne
   * @param speed: rychlost kterou robot pojede
   * @param distance: vzdalenost kterou ma robot ujet v mm
   * @param timeout: po jake dobe se ma funkce ukoncit i kdyz neujede pozadovanou vzdalenost
   * @return None
   */
  void Straight(int speed, int distance, int timeout)
  {
    man.motor(motorL).setCurrentPosition(0);
    man.motor(motorR).setCurrentPosition(0);
    int time = 0;
    int ticks_ML = 0;
    int ticks_MR = 0;
    distance = distance / mm_to_ticks;
    Serial.println(distance);
    while (ticks_ML < distance && time < timeout)
    {
      checkEmergencyStop();
      man.motor(motorL).speed(speed);
      man.motor(motorR).speed(-speed);
      man.motor(motorR).requestInfo([&ticks_MR](rb::Motor &info)
                                    { ticks_MR = -info.position(); });
      man.motor(motorL).requestInfo([&ticks_ML](rb::Motor &info)
                                    { ticks_ML = info.position(); });
      // Serial.printf("ML: %d, MR: %d\n", ticks_ML, ticks_MR);

      delay(10);
      time = time + 10;
    }
  }

  void Stop()
  {
    man.motor(motorL).speed(0);
    man.motor(motorR).speed(0);
  }
};

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <RBCX.h>

auto &man = rb::Manager::get(); // pro fungovani RBCX
#include "Grabber.hpp"
#include "Movement.hpp"
#include "Arm.hpp"
#include "Sensors.hpp"


Grabber grab;
Movement move;
Sensors sens;
Arm arm;


// Vytvoření nové sběrnice Wire1
extern TwoWire Wire1;


int final_time = 220;

byte smaller_red_count = 0;
byte smaller_green_count = 0;
byte smaller_blue_count = 0;
byte bigger_red_count = 0;
byte bigger_green_count = 0;
byte bigger_blue_count = 0;

void WaitForStart()
{
    while (true)
    {
        if (man.buttons().on() == 1)
        {
            break;
        }
        delay(10);
        if (man.buttons().up() == 1){
          arm.BiggerUp();
          arm.SmallerUp();
          move.BackwardUntillWall(20000);
          move.Straight(1000, 40, 1000);
          move.TurnLeft(88);
          move.BackwardUntillWall(3000);
          move.Acceleration(300, 10000, 250);
          move.ArcRight(90, 190);
          move.ArcLeft(180, 200);
          move.Straight(4000, 10000, 32000);
        }
    }
}

void CheckBattery()
{
    const auto &bat = man.battery();
    static const uint32_t VOLTAGE_MAX = 7800; //%edit
    static const uint32_t VOLTAGE_MIN = 7000; //%edit
    int i = 0;
    int voltage = 0;
    for (i = 0; i < 2; ++i)
    {
        voltage = bat.voltageMv();
        // Vypočítej procenta (omez na 0-100)
        int pct = (voltage - VOLTAGE_MIN) * 100 / (VOLTAGE_MAX - VOLTAGE_MIN);
        if (pct > 100)
            pct = 100;
        if (pct < 0)
            pct = 0;

        if (i > 0)
        {
            printf("Battery at %d%%, %dmv\n", pct, voltage);
            if (voltage < VOLTAGE_MIN)
            {
                printf("Je třeba nabít baterii!\n");
            }
        }
        delay(1000);
    }
}

void GoToField(){
  move.Acceleration(900, 10000, 500);
  Serial.println("go to field1");
  move.ArcRight(170,180);
  Serial.println("go to field2");
  //move.Straight(3200,100,5000);
  move.ArcLeft(150, 140);
  Serial.println("go to field3");
  move.Straight(32000, 1450,4000);
  Serial.println("go to field4");
  move.Acceleration(32000, 100, 320);
  Serial.println("go to field5");
  move.Stop();
}

void BrickDeliver(Color smaller_arm_brick, Color bigger_arm_brick)
{
    if (smaller_arm_brick == COLOR_RED)
    {
      move.TurnLeft(90);
      move.BackwardUntillWall();
      move.Straight(2500, 180+smaller_red_count*30, 1000); // musi se zkontrolovat, aby potom pri otaceni nanarazil cumakem do zdi
      move.Stop();
      move.TurnRight(90);
      move.BackwardUntillWall();
      arm.SmallerBack();
      if (bigger_arm_brick == COLOR_RED) arm.BiggerBack();
      delay(1000);
      if (bigger_arm_brick == COLOR_RED) grab.BiggerArmOpen();
      grab.SmallerArmOpen();
      delay(1000);

      move.Straight(2000, 100, 1000);
    }
    else if (smaller_arm_brick == COLOR_GREEN)
    {
      move.TurnLeft(90);
      move.BackwardUntillWall();
      move.Acceleration(600, 10000, 200);
      move.Straight(10000, 360+smaller_green_count*40, 5000);
      move.Stop();
      move.TurnRight(90);
      move.BackwardUntillWall();
      arm.SmallerBack();
      if (bigger_arm_brick == COLOR_GREEN) arm.BiggerBack();
      delay(1000);
      if (bigger_arm_brick == COLOR_GREEN) grab.BiggerArmOpen();
      grab.SmallerArmOpen();
      delay(1000);
      arm.SmallerUp();
      arm.BiggerUp();
      delay(200);
      move.Straight(2000, 100, 1000);
    }
    else // blue
    {
      move.TurnRight(90);
      move.BackwardUntillWall();
      move.Straight(2000, 210 +smaller_blue_count*30, 1000); // musi se zkontrolovat, aby potom pri otaceni nanarazil cumakem do zdi
      move.Stop();
      move.TurnLeft(90);
      move.BackwardUntillWall();
      arm.SmallerBack();
      if (bigger_arm_brick == COLOR_BLUE) arm.BiggerBack();
      delay(1000);
      if (bigger_arm_brick == COLOR_BLUE) grab.BiggerArmOpen();
      grab.SmallerArmOpen();
      delay(1000);
      arm.SmallerUp();
      arm.BiggerUp();
      delay(200);
      move.Straight(2000, 100, 1000);
    }
    if (bigger_arm_brick != smaller_arm_brick)
    {
      if (bigger_arm_brick == COLOR_RED)
      {
        move.TurnLeft(90);
        move.BackwardUntillWall();
        move.Straight(2000, 180 + bigger_red_count*30, 1000); // musi se zkontrolovat, aby potom pri otaceni nanarazil cumakem do zdi
        move.Stop();
        move.TurnRight(90);
        move.BackwardUntillWall();
        arm.BiggerBack();
        delay(1000);
        grab.BiggerArmOpen();
        delay(1000);
        arm.BiggerUp();
        delay(200);
        move.Straight(2000, 100, 1000);
      }
      else if (bigger_arm_brick == COLOR_GREEN)
      {
        move.TurnLeft(90);
        move.BackwardUntillWall();
        move.Acceleration(600, 10000, 200);
        move.Straight(10000, 360 + bigger_green_count*40, 5000);
        move.Stop();
        move.TurnRight(90);
        move.BackwardUntillWall();
        arm.BiggerBack();
        delay(1000);
        grab.BiggerArmOpen();
        delay(1000);
        arm.BiggerUp();
        delay(200);
        move.Straight(2000, 100, 1000);
      }
      else // blue
      {
        move.TurnRight(90);
        move.BackwardUntillWall();
        move.Straight(2000, 200 + bigger_blue_count*20, 1000);
        move.Stop();
        move.TurnLeft(90);
        move.BackwardUntillWall();
        arm.BiggerBack();
        delay(1000);
        grab.BiggerArmOpen();
        delay(1000);
        arm.BiggerUp();
        delay(200);
        move.Straight(2000, 100, 1000);
      }
    }
}


void setup(){

   auto &man = rb::Manager::get(); // get manager instance as singleton
    man.install();  // install manager, this will initialize the hardware and the
    
  Serial.begin(115200);
  delay(100);




  sens.InitRGB(); // Inicializace RGB senzorů

  servoBus.begin(2, UART_NUM_1, GPIO_NUM_27 ); // Inicializace sběrnice pro serva na GPIO 27 s UART1
  
  grab.BiggerArmOpen();
  grab.SmallerArmOpen();


  WaitForStart(); // Čekej na stisk tlačítka "ON" pro spuštění

  float start_time = millis()/1000; // Ulož čas spuštění v sekundách
  Serial.printf("Start time: %.2f seconds\n", start_time);

  arm.BiggerUp();
  arm.SmallerUp();

  Serial.println("Starting robot movement...");
  
  GoToField();

  move.TurnLeft(90);
  move.BackwardUntillWall();
  for (size_t lap = 0; (lap < 6) && (millis()/1000 - start_time < final_time); lap++)
  {
    move.Acceleration(500, 20000, 210);
    move.Straight(32000, 800 - lap*150, 10000); // Příkaz pro pohyb robota rovně na 1 metr s rychlostí 2500 a timeoutem 5 sekund
    move.Stop();
    move.TurnLeft(90); // Otoč robota doprava o 90 stupňů
    arm.SmallerFront();
    arm.BiggerFront();
    move.BackwardUntillWall();
    move.Straight(5000, 400, 5000);
    move.Straight(2500, 450, 5000);
    move.Stop();
    grab.SmallerArmClose();
    grab.BiggerArmClose();
    delay(1000);
    move.BackwardUntillWall();
    grab.SmallerArmOpen();
    grab.BiggerArmOpen();
    move.Straight(2500, 200, 4000);
    grab.SmallerArmClose();
    grab.BiggerArmClose();
    delay(1000);
    Color smaller_arm_brick = sens.GetColorRGB2();
    Color bigger_arm_brick = sens.GetColorRGB1();
    move.BackwardUntillWall();
    move.Straight(2000, 100, 1000);
    BrickDeliver(smaller_arm_brick, bigger_arm_brick);
    move.Stop();
    move.TurnRight(93);
    move.BackwardUntillWall();
  }
  arm.BiggerUp();
  arm.SmallerUp();
  move.Straight(1000, 40, 1000);
  move.TurnRight(88);
  move.BackwardUntillWall(17000);
  move.Straight(1000, 140, 1000);
  move.TurnLeft(88);
  move.BackwardUntillWall(3000);
  move.Acceleration(300, 10000, 250);
  move.ArcRight(90, 190);
  move.ArcLeft(170, 210);
  move.Straight(4000, 10000, 32000);

}

void loop() {

}

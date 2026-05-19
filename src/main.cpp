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

void checkButtons() {
    if (man.buttons().left() == 1) {
        sens.ReadRGB();
        const char* s1_col = (sens.r_1 > sens.g_1 && sens.r_1 > sens.b_1) ? "Cervena" : ((sens.g_1 > sens.r_1 && sens.g_1 > sens.b_1) ? "Zelena" : "Modra");
        const char* s2_col = (sens.r_2 > sens.g_2 && sens.r_2 > sens.b_2) ? "Cervena" : ((sens.g_2 > sens.r_2 && sens.g_2 > sens.b_2) ? "Zelena" : "Modra");
        Serial.printf("LEFT: S1 (Front) -> R: %.1f, G: %.1f, B: %.1f, Clear: %d (%s) | S2 (Down) -> R: %.1f, G: %.1f, B: %.1f, Clear: %d (%s)\n",
            sens.r_1, sens.g_1, sens.b_1, sens.clear_1, s1_col,
            sens.r_2, sens.g_2, sens.b_2, sens.clear_2, s2_col);
        delay(200);
    }

    if (man.buttons().right() == 1) {
        sens.ReadRGB();
        Serial.printf("RIGHT: Merim barvy... S1 Clear: %d, S2 Clear: %d\n", sens.clear_1, sens.clear_2);
        
        bool found_s1 = (sens.clear_1 < 1000);
        bool found_s2 = (sens.clear_2 < 1000);
        
        if (found_s1) {
            const char* col_name = (sens.GetColorRGB1() == COLOR_RED) ? "Cervena" : ((sens.GetColorRGB1() == COLOR_GREEN) ? "Zelena" : "Modra");
            Serial.printf("Predni senzor (S1) detekoval barvu: %s (Clear: %d v toleranci)\n", col_name, sens.clear_1);
            
            Serial.println("Zaviram velke klepeto...");
            grab.BiggerArmClose();
            delay(1000);
            
            Serial.println("Hazim kostku za sebe...");
            arm.BiggerBack();
            delay(1000);
            
            Serial.println("Oteviram velke klepeto vzadu...");
            grab.BiggerArmOpen();
            delay(1000);
            
            Serial.println("Vracim velke klepeto nahoru...");
            arm.BiggerUp();
            delay(1000);
        }
        
        if (found_s2) {
            const char* col_name = (sens.GetColorRGB2() == COLOR_RED) ? "Cervena" : ((sens.GetColorRGB2() == COLOR_GREEN) ? "Zelena" : "Modra");
            Serial.printf("Spodni senzor (S2) detekoval barvu: %s (Clear: %d v toleranci)\n", col_name, sens.clear_2);
            
            Serial.println("Zaviram male klepeto...");
            grab.SmallerArmClose();
            delay(1000);
            
            Serial.println("Hazim kostku za sebe...");
            arm.SmallerBack();
            delay(1000);
            
            Serial.println("Oteviram male klepeto vzadu...");
            grab.SmallerArmOpen();
            delay(1000);
            
            Serial.println("Vracim male klepeto nahoru...");
            arm.SmallerUp();
            delay(1000);
        }
        
        if (!found_s1 && !found_s2) {
            Serial.println("Zadna kostka v toleranci (clear < 1000) nebyla nalezena.");
        }
        
        delay(200);
    }
}

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
        checkButtons();
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
      Serial.println("Doručuji kostky: Jedu na ČERVENOU pozici pro malé klepeto.");
      move.TurnLeft(90);
      move.BackwardUntillWall();
      move.Straight(2500, 180+smaller_red_count*30, 1000); // musi se zkontrolovat, aby potom pri otaceni nanarazil cumakem do zdi
      move.Stop();
      move.TurnRight(90);
      move.BackwardUntillWall();
      Serial.println("Přesouvám malé rameno dozadu (červená).");
      arm.SmallerBack();
      if (bigger_arm_brick == COLOR_RED) {
        Serial.println("Přesouvám také velké rameno dozadu (červená).");
        arm.BiggerBack();
      }
      delay(1000);
      if (bigger_arm_brick == COLOR_RED) {
        Serial.println("Otevírám velké klepeto (červená).");
        grab.BiggerArmOpen();
      }
      Serial.println("Otevírám malé klepeto (červená).");
      grab.SmallerArmOpen();
      delay(1000);

      move.Straight(2000, 100, 1000);
    }
    else if (smaller_arm_brick == COLOR_GREEN)
    {
      Serial.println("Doručuji kostky: Jedu na ZELENOU pozici pro malé klepeto.");
      move.TurnLeft(90);
      move.BackwardUntillWall();
      move.Acceleration(600, 10000, 200);
      move.Straight(10000, 360+smaller_green_count*40, 5000);
      move.Stop();
      move.TurnRight(90);
      move.BackwardUntillWall();
      Serial.println("Přesouvám malé rameno dozadu (zelená).");
      arm.SmallerBack();
      if (bigger_arm_brick == COLOR_GREEN) {
        Serial.println("Přesouvám také velké rameno dozadu (zelená).");
        arm.BiggerBack();
      }
      delay(1000);
      if (bigger_arm_brick == COLOR_GREEN) {
        Serial.println("Otevírám velké klepeto (zelená).");
        grab.BiggerArmOpen();
      }
      Serial.println("Otevírám malé klepeto (zelená).");
      grab.SmallerArmOpen();
      delay(1000);
      Serial.println("Vracím obě ramena nahoru.");
      arm.SmallerUp();
      arm.BiggerUp();
      delay(200);
      move.Straight(2000, 100, 1000);
    }
    else // blue
    {
      Serial.println("Doručuji kostky: Jedu na MODROU pozici pro malé klepeto.");
      move.TurnRight(90);
      move.BackwardUntillWall();
      move.Straight(2000, 210 +smaller_blue_count*30, 1000); // musi se zkontrolovat, aby potom pri otaceni nanarazil cumakem do zdi
      move.Stop();
      move.TurnLeft(90);
      move.BackwardUntillWall();
      Serial.println("Přesouvám malé rameno dozadu (modrá).");
      arm.SmallerBack();
      if (bigger_arm_brick == COLOR_BLUE) {
        Serial.println("Přesouvám také velké rameno dozadu (modrá).");
        arm.BiggerBack();
      }
      delay(1000);
      if (bigger_arm_brick == COLOR_BLUE) {
        Serial.println("Otevírám velké klepeto (modrá).");
        grab.BiggerArmOpen();
      }
      Serial.println("Otevírám malé klepeto (modrá).");
      grab.SmallerArmOpen();
      delay(1000);
      Serial.println("Vracím obě ramena nahoru.");
      arm.SmallerUp();
      arm.BiggerUp();
      delay(200);
      move.Straight(2000, 100, 1000);
    }
    if (bigger_arm_brick != smaller_arm_brick)
    {
      if (bigger_arm_brick == COLOR_RED)
      {
        Serial.println("Doručuji kostky: Jedu na ČERVENOU pozici pro velké klepeto.");
        move.TurnLeft(90);
        move.BackwardUntillWall();
        move.Straight(2000, 180 + bigger_red_count*30, 1000); // musi se zkontrolovat, aby potom pri otaceni nanarazil cumakem do zdi
        move.Stop();
        move.TurnRight(90);
        move.BackwardUntillWall();
        Serial.println("Přesouvám velké rameno dozadu (červená).");
        arm.BiggerBack();
        delay(1000);
        Serial.println("Otevírám velké klepeto (červená).");
        grab.BiggerArmOpen();
        delay(1000);
        Serial.println("Vracím velké rameno nahoru.");
        arm.BiggerUp();
        delay(200);
        move.Straight(2000, 100, 1000);
      }
      else if (bigger_arm_brick == COLOR_GREEN)
      {
        Serial.println("Doručuji kostky: Jedu na ZELENOU pozici pro velké klepeto.");
        move.TurnLeft(90);
        move.BackwardUntillWall();
        move.Acceleration(600, 10000, 200);
        move.Straight(10000, 360 + bigger_green_count*40, 5000);
        move.Stop();
        move.TurnRight(90);
        move.BackwardUntillWall();
        Serial.println("Přesouvám velké rameno dozadu (zelená).");
        arm.BiggerBack();
        delay(1000);
        Serial.println("Otevírám velké klepeto (zelená).");
        grab.BiggerArmOpen();
        delay(1000);
        Serial.println("Vracím velké rameno nahoru.");
        arm.BiggerUp();
        delay(200);
        move.Straight(2000, 100, 1000);
      }
      else // blue
      {
        Serial.println("Doručuji kostky: Jedu na MODROU pozici pro velké klepeto.");
        move.TurnRight(90);
        move.BackwardUntillWall();
        move.Straight(2000, 200 + bigger_blue_count*20, 1000);
        move.Stop();
        move.TurnLeft(90);
        move.BackwardUntillWall();
        Serial.println("Přesouvám velké rameno dozadu (modrá).");
        arm.BiggerBack();
        delay(1000);
        Serial.println("Otevírám velké klepeto (modrá).");
        grab.BiggerArmOpen();
        delay(1000);
        Serial.println("Vracím velké rameno nahoru.");
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

  Serial.println("Robot odstartoval. Inicializuji polohu ramen.");
  arm.BiggerUp();
  arm.SmallerUp();

  Serial.println("Jedu na pole (GoToField)...");
  GoToField();

  Serial.println("Otačení a couvání k zadní stěně...");
  move.TurnLeft(90);
  move.BackwardUntillWall();
  for (size_t lap = 0; (lap < 6) && (millis()/1000 - start_time < final_time); lap++)
  {
    Serial.printf("\n--- Start kola %d ---\n", lap + 1);
    Serial.println("Jedu dopředu pro vyhledání kostek...");
    move.Acceleration(500, 20000, 210);
    move.Straight(32000, 800 - lap*150, 10000); // Příkaz pro pohyb robota rovně na 1 metr s rychlostí 2500 a timeoutem 5 sekund
    move.Stop();
    
    Serial.println("Otáčím se k nakládací rampě/stěně.");
    move.TurnLeft(90); // Otoč robota doprava o 90 stupňů
    
    Serial.println("Sklápím obě ramena dopředu.");
    arm.SmallerFront();
    arm.BiggerFront();
    
    Serial.println("Najíždím ke kostkám a zastavuji před nimi.");
    move.BackwardUntillWall();
    move.Straight(5000, 400, 5000);
    move.Straight(2500, 450, 5000);
    move.Stop();
    
    Serial.println("Zavírám malé i velké klepeto pro první uchopení.");
    grab.SmallerArmClose();
    grab.BiggerArmClose();
    delay(1000);
    
    Serial.println("Couvám ke stěně a otevírám klepeta.");
    move.BackwardUntillWall();
    grab.SmallerArmOpen();
    grab.BiggerArmOpen();
    
    Serial.println("Najíždím kousek dopředu a definitivně zavírám klepeta pro uchopení kostek.");
    move.Straight(2500, 200, 4000);
    grab.SmallerArmClose();
    grab.BiggerArmClose();
    delay(1000);
    
    Color smaller_arm_brick = sens.GetColorRGB2();
    Color bigger_arm_brick = sens.GetColorRGB1();
    const char* s_color = (smaller_arm_brick == COLOR_RED) ? "červená" : ((smaller_arm_brick == COLOR_GREEN) ? "zelená" : "modrá");
    const char* b_color = (bigger_arm_brick == COLOR_RED) ? "červená" : ((bigger_arm_brick == COLOR_GREEN) ? "zelená" : "modrá");
    Serial.printf("V malém klepetu jsem chytil %s barvu.\n", s_color);
    Serial.printf("V dlouhém (velkém) klepetu jsem chytil %s barvu.\n", b_color);
    
    move.BackwardUntillWall();
    move.Straight(2000, 100, 1000);
    BrickDeliver(smaller_arm_brick, bigger_arm_brick);
    move.Stop();
    move.TurnRight(93);
    move.BackwardUntillWall();
  }
  Serial.println("Konec hlavní smyčky. Zvedám ramena a jedu domů.");
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
  Serial.println("Robot se vrátil domů. Konec programu.");

}

void loop() {
  checkButtons();
  delay(10);
}

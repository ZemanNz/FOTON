#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <RBCX.h>
#include <stdarg.h>

auto &man = rb::Manager::get(); // pro fungovani RBCX

String logBuffer = "";

void logMsg(const char* format, ...) {
    char temp[256];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    
    logBuffer += temp;
    logBuffer += "\n";
    Serial.println(temp);
}

// UART komunikace s externím ESP32 pro gyroskop (MPU-6050)
#define GYRO_UART_RX 16
#define GYRO_UART_TX 17
#define GYRO_UART_BAUD 115200

volatile float gyro_angle_z = 0.0f;

struct __attribute__((__packed__)) GyroPacket {
    uint8_t sync1; // První synchronizační bajt (0x12)
    uint8_t sync2; // Druhý synchronizační bajt (0x34)
    float angleZ;  // Relativní úhel otočení v ose Z (Heading) ve stupních
};

struct __attribute__((__packed__)) CommandPacket {
    uint8_t sync1; // První synchronizační bajt (0x56)
    uint8_t sync2; // Druhý synchronizační bajt (0x78)
    uint8_t cmd;   // Příkaz (např. 'R' pro reset Z)
};

void updateGyroUart() {
    while (Serial1.available() >= sizeof(GyroPacket)) {
        if (Serial1.peek() != 0x12) {
            uint8_t discarded = Serial1.read();
            Serial.printf("UART RAW: Zahozeny neplatny bajt: 0x%02X\n", discarded);
            continue;
        }
        
        uint8_t s1 = Serial1.read(); // sync1 (0x12)
        if (Serial1.peek() != 0x34) {
            Serial.printf("UART RAW: Chyba sync2! Nasel jsem sync1=0x12, ale dalsi je 0x%02X\n", Serial1.peek());
            continue; 
        }
        uint8_t s2 = Serial1.read(); // sync2 (0x34)
        
        float angle = 0.0f;
        Serial1.readBytes((uint8_t*)&angle, sizeof(angle));
        gyro_angle_z = angle;
        
        Serial.printf("UART RAW: Prijat paket -> Sync: [0x%02X, 0x%02X], Uhel Z: %.2f deg\n", s1, s2, angle);
    }
}

void resetGyroZ() {
    CommandPacket pkt;
    pkt.sync1 = 0x56;
    pkt.sync2 = 0x78;
    pkt.cmd = 'R';
    Serial1.write((const uint8_t*)&pkt, sizeof(pkt));
    gyro_angle_z = 0.0f;
}

void checkEmergencyStop() {
    if (man.buttons().down() == 1) {
        man.motor(rb::MotorId::M1).speed(0);
        man.motor(rb::MotorId::M2).speed(0);
        man.motor(rb::MotorId::M3).speed(0);
        man.motor(rb::MotorId::M4).speed(0);
        
        Serial.println("\n--- EMERGENCY STOP - VYPIS ULOZENYCH ZAZNAMU ---");
        Serial.print(logBuffer);
        Serial.println("--- KONEC VYPISU ---\n");
        Serial.println("Robot je zastaven. Resetujte pro novy start.");
        
        while(true) {
            ::delay(100);
            if (man.buttons().down() == 1) {
                Serial.println("\n--- VYPIS ULOZENYCH ZAZNAMU (LOG BUFFER) ---");
                Serial.print(logBuffer);
                Serial.println("--- KONEC VYPISU ---\n");
                ::delay(500);
            }
        }
    }
}

void safeDelay(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) {
        updateGyroUart(); // Průběžně vyčítáme data z UARTu
        checkEmergencyStop();
        ::delay(10);
    }
}

#include "Grabber.hpp"
#include "Movement.hpp"
#include "Arm.hpp"
#include "Sensors.hpp"

// Redirect all delay() calls to safeDelay() so that they also check for emergency stop
// (defined after includes to avoid conflicting with system and library headers)
#define delay safeDelay

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
        Color col1 = sens.GetColorRGB1();
        Color col2 = sens.GetColorRGB2();
        const char* s1_col = (col1 == COLOR_RED) ? "Cervena" : ((col1 == COLOR_GREEN) ? "Zelena" : "Modra");
        const char* s2_col = (col2 == COLOR_RED) ? "Cervena" : ((col2 == COLOR_GREEN) ? "Zelena" : "Modra");
        Serial.printf("LEFT: S1 (Front) -> R: %.1f, G: %.1f, B: %.1f, Clear: %d (%s) | S2 (Down) -> R: %.1f, G: %.1f, B: %.1f, Clear: %d (%s)\n",
            sens.r_1, sens.g_1, sens.b_1, sens.clear_1, s1_col,
            sens.r_2, sens.g_2, sens.b_2, sens.clear_2, s2_col);
        delay(200);
    }

    if (man.buttons().right() == 1) {
        sens.ReadRGB();
        logMsg("RIGHT: Merim barvy... S1 Clear: %d, S2 Clear: %d", sens.clear_1, sens.clear_2);
        
        bool found_s1 = (sens.clear_1 < 1000);
        bool found_s2 = (sens.clear_2 < 1000);
        
        if (found_s1) {
            const char* col_name = (sens.GetColorRGB1() == COLOR_RED) ? "Cervena" : ((sens.GetColorRGB1() == COLOR_GREEN) ? "Zelena" : "Modra");
            logMsg("Predni senzor (S1) detekoval barvu: %s (Clear: %d v toleranci)", col_name, sens.clear_1);
            
            logMsg("Zaviram velke klepeto...");
            grab.BiggerArmClose();
            delay(1000);
            
            logMsg("Hazim kostku za sebe...");
            arm.BiggerBack();
            delay(1000);
            
            logMsg("Oteviram velke klepeto vzadu...");
            grab.BiggerArmOpen();
            delay(1000);
            
            logMsg("Vracim velke klepeto nahoru...");
            arm.BiggerUp();
            delay(1000);
        }
        
        if (found_s2) {
            const char* col_name = (sens.GetColorRGB2() == COLOR_RED) ? "Cervena" : ((sens.GetColorRGB2() == COLOR_GREEN) ? "Zelena" : "Modra");
            logMsg("Spodni senzor (S2) detekoval barvu: %s (Clear: %d v toleranci)", col_name, sens.clear_2);
            
            logMsg("Zaviram male klepeto...");
            grab.SmallerArmClose();
            delay(1000);
            
            logMsg("Hazim kostku za sebe...");
            arm.SmallerBack();
            delay(1000);
            
            logMsg("Oteviram male klepeto vzadu...");
            grab.SmallerArmOpen();
            delay(1000);
            
            logMsg("Vracim male klepeto nahoru...");
            arm.SmallerUp();
            delay(1000);
        }
        
        if (!found_s1 && !found_s2) {
            logMsg("Zadna kostka v toleranci (clear < 1000) nebyla nalezena.");
        }
        
        delay(200);
    }

    if (man.buttons().down() == 1) {
        if (logBuffer.length() > 0) {
            Serial.println("\n--- VYPIS ULOZENYCH ZAZNAMU (LOG BUFFER) ---");
            Serial.print(logBuffer);
            Serial.println("--- KONEC VYPISU ---\n");
        } else {
            Serial.println("\n--- LOG BUFFER JE PRAZDNY ---\n");
        }
        delay(500);
    }
}

void WaitForStart()
{
    uint32_t last_print = 0;
    while (true)
    {
        if (man.buttons().on() == 1)
        {
            logBuffer = ""; // Reset log buffer for the new run
            break;
        }
        delay(2);
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
        
        // Pravé tlačítko: otestování klepeta na dlouhém rameni (Bigger arm)
        if (man.buttons().right() == 1) {
            grab.BiggerArmClose();
            delay(1000);
            grab.BiggerArmOpen();
            delay(200);
        }
        
        // Levé tlačítko: po sekundové pauze se otočí o 90 stupňů doleva
        if (man.buttons().left() == 1) {
            delay(1000);
            move.TurnLeft(90);
        }
        
        // Spodní tlačítko (DOWN): výpis uloženého log bufferu
        if (man.buttons().down() == 1) {
            if (logBuffer.length() > 0) {
                Serial.println("\n--- VYPIS ULOZENYCH ZAZNAMU (LOG BUFFER) ---");
                Serial.print(logBuffer);
                Serial.println("--- KONEC VYPISU ---\n");
            } else {
                Serial.println("\n--- LOG BUFFER JE PRAZDNY ---\n");
            }
            delay(500);
        }
        
        // Vypisování hodnoty z předního RGB senzoru (S1 na rameni) a gyroskopu každých 300 ms po startu desky
        if (millis() - last_print >= 300) {
            Color col1 = sens.GetColorRGB1();
            updateGyroUart();
            const char* s1_col = (col1 == COLOR_RED) ? "Cervena" : ((col1 == COLOR_GREEN) ? "Zelena" : "Modra");
            Serial.printf("FRONT S1: R: %.1f, G: %.1f, B: %.1f, Clear: %d (%s) | Gyro Z (UART): %.2f deg\n",
                sens.r_1, sens.g_1, sens.b_1, sens.clear_1, s1_col, gyro_angle_z);
            last_print = millis();
        }
    }
}

void CheckBattery()
{
    const auto &bat = man.battery();
    static const uint32_t VOLTAGE_MAX = 8800; //%edit
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
  logMsg("go to field1");
  move.ArcRight(170,180);
  logMsg("go to field2");
  //move.Straight(3200,100,5000);
  move.ArcLeft(150, 140);
  logMsg("go to field3");
  move.Straight(32000, 1450,4000);
  logMsg("go to field4");
  move.Acceleration(32000, 100, 320);
  logMsg("go to field5");
  move.Stop();
}

void BrickDeliver(Color smaller_arm_brick, Color bigger_arm_brick)
{
    if (smaller_arm_brick == COLOR_RED)
    {
      logMsg("Doručuji kostky: Jedu na ČERVENOU pozici pro malé klepeto.");
      move.TurnLeft(90);
      move.BackwardUntillWall();
      move.Straight(2500, 180+smaller_red_count*30, 1000); // musi se zkontrolovat, aby potom pri otaceni nanarazil cumakem do zdi
      move.Stop();
      move.TurnRight(90);
      move.BackwardUntillWall();
      logMsg("Přesouvám malé rameno dozadu (červená).");
      arm.SmallerBack();
      if (bigger_arm_brick == COLOR_RED) {
        logMsg("Přesouvám také velké rameno dozadu (červená).");
        arm.BiggerBack();
      }
      delay(1000);
      if (bigger_arm_brick == COLOR_RED) {
        logMsg("Otevírám velké klepeto (červená).");
        grab.BiggerArmOpen();
      }
      logMsg("Otevírám malé klepeto (červená).");
      grab.SmallerArmOpen();
      delay(1000);
      logMsg("Vracím obě ramena nahoru.");
      arm.SmallerUp();
      arm.BiggerUp();
      delay(200);

      move.Straight(2000, 100, 1000);
    }
    else if (smaller_arm_brick == COLOR_GREEN)
    {
      logMsg("Doručuji kostky: Jedu na ZELENOU pozici pro malé klepeto.");
      move.TurnLeft(90);
      move.BackwardUntillWall();
      move.Acceleration(600, 10000, 200);
      move.Straight(10000, 360+smaller_green_count*40, 5000);
      move.Stop();
      move.TurnRight(90);
      move.BackwardUntillWall();
      logMsg("Přesouvám malé rameno dozadu (zelená).");
      arm.SmallerBack();
      if (bigger_arm_brick == COLOR_GREEN) {
        logMsg("Přesouvám také velké rameno dozadu (zelená).");
        arm.BiggerBack();
      }
      delay(1000);
      if (bigger_arm_brick == COLOR_GREEN) {
        logMsg("Otevírám velké klepeto (zelená).");
        grab.BiggerArmOpen();
      }
      logMsg("Otevírám malé klepeto (zelená).");
      grab.SmallerArmOpen();
      delay(1000);
      logMsg("Vracím obě ramena nahoru.");
      arm.SmallerUp();
      arm.BiggerUp();
      delay(200);
      move.Straight(2000, 100, 1000);
    }
    else // blue
    {
      logMsg("Doručuji kostky: Jedu na MODROU pozici pro malé klepeto.");
      move.TurnRight(90);
      move.BackwardUntillWall();
      move.Straight(2000, 210 +smaller_blue_count*30, 1000); // musi se zkontrolovat, aby potom pri otaceni nanarazil cumakem do zdi
      move.Stop();
      move.TurnLeft(90);
      move.BackwardUntillWall();
      logMsg("Přesouvám malé rameno dozadu (modrá).");
      arm.SmallerBack();
      if (bigger_arm_brick == COLOR_BLUE) {
        logMsg("Přesouvám také velké rameno dozadu (modrá).");
        arm.BiggerBack();
      }
      delay(1000);
      if (bigger_arm_brick == COLOR_BLUE) {
        logMsg("Otevírám velké klepeto (modrá).");
        grab.BiggerArmOpen();
      }
      logMsg("Otevírám malé klepeto (modrá).");
      grab.SmallerArmOpen();
      delay(1000);
      logMsg("Vracím obě ramena nahoru.");
      arm.SmallerUp();
      arm.BiggerUp();
      delay(200);
      move.Straight(2000, 100, 1000);
    }
    if (bigger_arm_brick != smaller_arm_brick)
    {
      if (bigger_arm_brick == COLOR_RED)
      {
        logMsg("Doručuji kostky: Jedu na ČERVENOU pozici pro velké klepeto.");
        move.TurnLeft(90);
        move.BackwardUntillWall();
        move.Straight(2000, 180 + bigger_red_count*30, 1000); // musi se zkontrolovat, aby potom pri otaceni nanarazil cumakem do zdi
        move.Stop();
        move.TurnRight(90);
        move.BackwardUntillWall();
        logMsg("Přesouvám velké rameno dozadu (červená).");
        arm.BiggerBack();
        delay(1000);
        logMsg("Otevírám velké klepeto (červená).");
        grab.BiggerArmOpen();
        delay(1000);
        logMsg("Vracím velké rameno nahoru.");
        arm.BiggerUp();
        delay(200);
        move.Straight(2000, 100, 1000);
      }
      else if (bigger_arm_brick == COLOR_GREEN)
      {
        logMsg("Doručuji kostky: Jedu na ZELENOU pozici pro velké klepeto.");
        move.TurnLeft(90);
        move.BackwardUntillWall();
        move.Acceleration(600, 10000, 200);
        move.Straight(10000, 360 + bigger_green_count*40, 5000);
        move.Stop();
        move.TurnRight(90);
        move.BackwardUntillWall();
        logMsg("Přesouvám velké rameno dozadu (zelená).");
        arm.BiggerBack();
        delay(1000);
        logMsg("Otevírám velké klepeto (zelená).");
        grab.BiggerArmOpen();
        delay(1000);
        logMsg("Vracím velké rameno nahoru.");
        arm.BiggerUp();
        delay(200);
        move.Straight(2000, 100, 1000);
      }
      else // blue
      {
        logMsg("Doručuji kostky: Jedu na MODROU pozici pro velké klepeto.");
        move.TurnRight(90);
        move.BackwardUntillWall();
        move.Straight(2000, 200 + bigger_blue_count*20, 1000);
        move.Stop();
        move.TurnLeft(90);
        move.BackwardUntillWall();
        logMsg("Přesouvám velké rameno dozadu (modrá).");
        arm.BiggerBack();
        delay(1000);
        logMsg("Otevírám velké klepeto (modrá).");
        grab.BiggerArmOpen();
        delay(1000);
        logMsg("Vracím velké rameno nahoru.");
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

  // Inicializace UART1 pro komunikaci s gyroskopem na externím ESP32
  Serial1.begin(GYRO_UART_BAUD, SERIAL_8N1, GYRO_UART_RX, GYRO_UART_TX);
  Serial.printf("Serial1 (Gyro) spusten na RX=%d, TX=%d\n", GYRO_UART_RX, GYRO_UART_TX);

  sens.InitRGB(); // Inicializace RGB senzorů

  // servoBus.begin(2, UART_NUM_1, GPIO_NUM_27 ); // Inicializace sběrnice pro serva na GPIO 27 s UART1
  
  grab.BiggerArmOpen();
  grab.SmallerArmOpen();


  WaitForStart(); // Čekej na stisk tlačítka "ON" pro spuštění

  float start_time = millis()/1000; // Ulož čas spuštění v sekundách
  logMsg("Start time: %.2f seconds", start_time);

  logMsg("Robot odstartoval. Inicializuji polohu ramen.");
  arm.BiggerUp();
  arm.SmallerUp();

  logMsg("Jedu na pole (GoToField)...");
  GoToField();

  logMsg("Otačení a couvání k zadní stěně...");
  move.TurnLeft(90);
  move.BackwardUntillWall();
  for (size_t lap = 0; (lap < 6) && (millis()/1000 - start_time < final_time); lap++)
  {
    logMsg("\n--- Start kola %d ---", lap + 1);
    logMsg("Jedu dopředu pro vyhledání kostek...");
    move.Acceleration(500, 20000, 210);
    move.Straight(32000, 800 - lap*150, 10000); // Příkaz pro pohyb robota rovně na 1 metr s rychlostí 2500 a timeoutem 5 sekund
    move.Stop();
    
    logMsg("Otáčím se k nakládací rampě/stěně.");
    move.TurnLeft(90); // Otoč robota doprava o 90 stupňů
    
    logMsg("Sklápím obě ramena dopředu.");
    arm.SmallerFront();
    arm.BiggerFront();
    
    logMsg("Najíždím ke kostkám a zastavuji před nimi.");
    move.BackwardUntillWall();
    move.Straight(5000, 400, 5000);
    move.Straight(2500, 450, 5000);
    move.Stop();
    
    logMsg("Zavírám malé i velké klepeto pro první uchopení.");
    grab.SmallerArmClose();
    grab.BiggerArmClose();
    delay(1000);
    
    logMsg("Couvám ke stěně a otevírám klepeta.");
    move.BackwardUntillWall();
    grab.SmallerArmOpen();
    grab.BiggerArmOpen();
    
    logMsg("Najíždím kousek dopředu a definitivně zavírám klepeta pro uchopení kostek.");
    move.Straight(2500, 200, 4000);
    grab.SmallerArmClose();
    grab.BiggerArmClose();
    delay(1000);
    
    Color smaller_arm_brick = sens.GetColorRGB2();
    Color bigger_arm_brick = sens.GetColorRGB1();
    const char* s_color = (smaller_arm_brick == COLOR_RED) ? "červená" : ((smaller_arm_brick == COLOR_GREEN) ? "zelená" : "modrá");
    const char* b_color = (bigger_arm_brick == COLOR_RED) ? "červená" : ((bigger_arm_brick == COLOR_GREEN) ? "zelená" : "modrá");
    logMsg("V malém klepetu jsem chytil %s barvu.", s_color);
    logMsg("V dlouhém (velkém) klepetu jsem chytil %s barvu.", b_color);
    
    move.BackwardUntillWall();
    move.Straight(2000, 100, 1000);
    BrickDeliver(smaller_arm_brick, bigger_arm_brick);
    move.Stop();
    move.TurnRight(93);
    move.BackwardUntillWall();
  }
  logMsg("Konec hlavní smyčky. Zvedám ramena a jedu domů.");
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
  logMsg("Robot se vrátil domů. Konec programu.");

}

void loop() {
  updateGyroUart();
  checkButtons();
  
  // Pravidelný výpis obdrženého úhlu na sériový monitor
  static unsigned long last_print = 0;
  if (millis() - last_print >= 250) {
      last_print = millis();
      Serial.printf("RBCX Prijato -> Gyro Z: %.2f deg\n", gyro_angle_z);
  }
  
  delay(10);
}

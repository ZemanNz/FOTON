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
            (void)Serial1.read();
            continue;
        }
        
        (void)Serial1.read(); // sync1 (0x12)
        if (Serial1.peek() != 0x34) {
            continue; 
        }
        (void)Serial1.read(); // sync2 (0x34)
        
        float angle = 0.0f;
        Serial1.readBytes((uint8_t*)&angle, sizeof(angle));
        gyro_angle_z = angle;
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

// Dopředná deklarace
void BrickDeliver(Color smaller_arm_brick, Color bigger_arm_brick, int lap);

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
void DriveToBricksWithSensors(int speed, int max_distance_mm, int timeout_ms) {
    man.motor(move.motorL).setCurrentPosition(0);
    man.motor(move.motorR).setCurrentPosition(0);
    
    int time = 0;
    int ticks_ML = 0;
    int ticks_MR = 0;
    int distance_ticks = max_distance_mm / move.mm_to_ticks;
    
    logMsg("Jedem pro kostky s aktivnim sledovanim infra senzoru. Max: %d mm", max_distance_mm);
    
    bool detected_both = false;
    int ticks_at_detection = 0;
    int reserve_ticks = 50 / move.mm_to_ticks; // 50 mm = 5 cm rezerva
    
    while (ticks_ML < distance_ticks && time < timeout_ms) {
        checkEmergencyStop();
        
        man.motor(move.motorL).speed(speed);
        man.motor(move.motorR).speed(-speed);
        
        man.motor(move.motorR).requestInfo([&ticks_MR](rb::Motor &info) {
            ticks_MR = -info.position();
        });
        man.motor(move.motorL).requestInfo([&ticks_ML](rb::Motor &info) {
            ticks_ML = info.position();
        });
        
        // Přečtení infračervených senzorů analogově pomocí funkcí ze Sensors.hpp.
        bool brick1 = sens.IsCubeInBiggerArmIR();
        bool brick2 = sens.IsCubeInSmallerArmIR();

        // Vypisujeme každých 100 ms pro přehlednost v logu
        if (time % 100 == 0) {
            printf("Pozice: %d mm, Kostka Delsi(IR): %d, Kostka Kratsi(IR): %d\n", 
                   (int)(ticks_ML * move.mm_to_ticks), brick1, brick2);
        }
        
        if (brick1 && brick2) {
            if (!detected_both) {
                detected_both = true;
                ticks_at_detection = ticks_ML;
                logMsg("Detekovany obe kostky infra senzory! Jedu rezervu 5 cm.");
            }
        }
        
        if (detected_both) {
            if (ticks_ML >= ticks_at_detection + reserve_ticks) {
                logMsg("Rezerva 5 cm ujeta. Zastavuji jizdu.");
                break;
            }
        }
        
        delay(10);
        time += 10;
    }
    move.Stop();
}

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
    while (true)
    {
        if (man.buttons().on() == 1)
        {
            logBuffer = ""; // Reset log buffer for the new run
            break;
        }
        delay(2);
        if (man.buttons().up() == 1){
          int lap = 1; // Budeme simulovat levé kolo (lap <= 3)
          
          logMsg("\n--- Start TESTOVACIHO kola (UP) ---");
          logMsg("Jedu dopředu pro vyhledání kostek...");
          move.Acceleration(500, 20000, 210);
          move.Straight(32000, 800 - lap*150, 10000);
          move.Stop();
          
          logMsg("Otáčím se k nakládací rampě/stěně.");
          move.TurnLeft(90);
          
          logMsg("Sklápím obě ramena dopředu.");
          arm.SmallerFront();
          arm.BiggerFront();
          
          logMsg("Najíždím ke kostkám a zastavuji před nimi.");
          move.BackwardUntillWall();
          move.Straight(5000, 300, 3000);
          DriveToBricksWithSensors(2500, 550, 6000);
          
          logMsg("Zavírám malé i velké klepeto pro první uchopení.");
          grab.SmallerArmClose();
          grab.BiggerArmClose();
          delay(1000);
          
          logMsg("Couvám ke stěně a otevírám klepeta.");
          move.BackwardUntillWall();
          grab.SmallerArmOpen();
          grab.BiggerArmOpen();
          
          logMsg("Najíždím kousek dopředu a definitivně zavírám klepeta pro uchopení kostek.");
          move.Straight(2000, 150, 2000);
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
          BrickDeliver(smaller_arm_brick, bigger_arm_brick, lap);
          grab.SmallerArmOpen();
          grab.BiggerArmOpen();
          
          move.Stop();
          move.TurnRight(90);
          move.BackwardUntillWall();
        }
        
        // Pravé tlačítko: otestování infra detekce pro menší (krátké) rameno
        if (man.buttons().right() == 1) {
            bool kostka = sens.IsCubeInSmallerArmIR();
            if (kostka) {
                Serial.println("Kostka JE v mensim klepetu (podle IR)!");
            } else {
                Serial.println("Kostka NENI v mensim klepetu (podle IR).");
            }
            delay(500);
        }
        
        // Levé tlačítko: otestování infra detekce pro větší (dlouhé) rameno
        if (man.buttons().left() == 1) {
            bool kostka = sens.IsCubeInBiggerArmIR();
            if (kostka) {
                Serial.println("Kostka JE ve vetsim klepetu (podle IR)!");
            } else {
                Serial.println("Kostka NENI ve vetsim klepetu (podle IR).");
            }
            delay(500);
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
  move.Straight(3200,40,5000);
  move.ArcLeft(160, 140);
  logMsg("go to field3");
  move.Straight(32000, 1150,4000);
  logMsg("go to field4");
  move.Acceleration(32000, 100, 320);
  logMsg("go to field5");
  move.Stop();
}

bool bliz_prava = false;
bool bliz_leva = false;

// Vzdálenosti pro doručování kostek (v mm od stěny, o kterou se robot opře)
int DIST_SHORT = 180;  // Krátká vzdálenost (pro modrou nebo červenou, pokud jsme u příslušné stěny)
int DIST_MID   = 600;  // Střední vzdálenost (vždy pro zelenou uprostřed)
int DIST_LONG  = 1000; // Dlouhá vzdálenost (zhruba metr, pokud přejíždíme k červené/modré z opačné stěny)

void BrickDeliver(Color smaller_arm_brick, Color bigger_arm_brick, int lap){
    bool is_left_wall = (lap <= 4); // lap 1, 2, 3 znamená levá stěna (ta u červené poličky)

    // SPECIÁLNÍ PŘÍPADY - Vhození dvou kostek na pomezí
    int target_dist = -1;

    // Od levé stěny
    if (is_left_wall) {
        if (bigger_arm_brick == COLOR_RED && smaller_arm_brick == COLOR_GREEN) {
            target_dist = (DIST_SHORT + DIST_MID) / 2; // pomezí červené a zelené (390)
        } else if (bigger_arm_brick == COLOR_GREEN && smaller_arm_brick == COLOR_BLUE) {
            target_dist = ((DIST_MID + DIST_LONG) / 2) + 40; // pomezí zelené a modré (800 + 400 = 1200 mm)
        }
    } 
    // Od pravé stěny (pro úplnost, symetricky)
    else {
        if (bigger_arm_brick == COLOR_GREEN && smaller_arm_brick == COLOR_BLUE) {
            target_dist = (DIST_SHORT + DIST_MID) / 2; // pomezí modré a zelené (390 zprava)
        } else if (bigger_arm_brick == COLOR_RED && smaller_arm_brick == COLOR_GREEN) {
            target_dist = ((DIST_MID + DIST_LONG) / 2) + 40; // pomezí zelené a červené (800 + 40 = 840 mm zprava)
        }
    }

    // Pokud došlo k jedné ze speciálních situací
    if (target_dist != -1) {
        logMsg("Specialni pripad: Vyhazuji obe kostky najednou na pomezi (vzdalenost %d).", target_dist);
        
        is_left_wall ? move.TurnLeft(90) : move.TurnRight(90);
        move.BackwardUntillWall();
        
        move.Straight(2500, target_dist, 6000);
        move.Stop();
        
        is_left_wall ? move.TurnRight(90) : move.TurnLeft(90);
        move.BackwardUntillWall();
        
        logMsg("Přesouvám obě ramena dozadu (vyhoz na pomezi).");
        arm.SmallerBack();
        arm.BiggerBack();
        delay(1000);
        grab.SmallerArmOpen();
        grab.BiggerArmOpen();
        delay(1000);
        logMsg("Vracím obě ramena nahoru.");
        arm.SmallerUp();
        arm.BiggerUp();
        delay(200);
        
        move.Straight(2000, 100, 1000);
        
        // Aktualizace globálních proměnných pro případné další využití
        if (target_dist < 500) {
            bliz_leva = is_left_wall;
            bliz_prava = !is_left_wall;
        } else {
            bliz_leva = !is_left_wall;
            bliz_prava = is_left_wall;
        }
        return;
    }

    struct Delivery {
        Color color;
        bool is_smaller_arm;
        int distance;
    };

    // Calculate distances
    int dist_small = -1;
    if (smaller_arm_brick != NIC) {
        if (smaller_arm_brick == COLOR_RED) {
            dist_small = is_left_wall ? (DIST_SHORT + smaller_red_count * 30) : (DIST_LONG + smaller_red_count * 30);
        } else if (smaller_arm_brick == COLOR_GREEN) {
            dist_small = DIST_MID + smaller_green_count * 40;
        } else if (smaller_arm_brick == COLOR_BLUE) {
            dist_small = is_left_wall ? (DIST_LONG + smaller_blue_count * 30) : (DIST_SHORT + smaller_blue_count * 30);
        }
    }

    int dist_big = -1;
    if (bigger_arm_brick != NIC) {
        if (bigger_arm_brick == COLOR_RED) {
            dist_big = is_left_wall ? (DIST_SHORT + bigger_red_count * 30) : (DIST_LONG + bigger_red_count * 30);
        } else if (bigger_arm_brick == COLOR_GREEN) {
            dist_big = DIST_MID + bigger_green_count * 40;
        } else if (bigger_arm_brick == COLOR_BLUE) {
            dist_big = is_left_wall ? (DIST_LONG + bigger_blue_count * 20) : (DIST_SHORT + bigger_blue_count * 20);
        }
    }

    Delivery first = { NIC, true, 0 };
    Delivery second = { NIC, false, 0 };

    if (smaller_arm_brick != NIC && bigger_arm_brick != NIC) {
        // Obě ramena mají kostku
        if (dist_small <= dist_big) {
            first = { smaller_arm_brick, true, dist_small };
            second = { bigger_arm_brick, false, dist_big };
        } else {
            first = { bigger_arm_brick, false, dist_big };
            second = { smaller_arm_brick, true, dist_small };
        }
    } else if (smaller_arm_brick != NIC) {
        // Pouze malé rameno má kostku
        first = { smaller_arm_brick, true, dist_small };
    } else if (bigger_arm_brick != NIC) {
        // Pouze velké rameno má kostku
        first = { bigger_arm_brick, false, dist_big };
    }

    if (first.color == NIC) {
        delay(200);
        move.Straight(2000, 100, 1000);
    } else {
        // 1. Doručení (bližší kostka)
        const char* color_name1 = (first.color == COLOR_RED) ? "ČERVENOU" : ((first.color == COLOR_GREEN) ? "ZELENOU" : "MODROU");
        logMsg("Doručuji kostky: Jedu na %s pozici (1. krok).", color_name1);
        
        is_left_wall ? move.TurnLeft(90) : move.TurnRight(90);
        move.BackwardUntillWall();
        
        move.Straight(2500, first.distance, 6000);
        move.Stop();
        
        is_left_wall ? move.TurnRight(90) : move.TurnLeft(90);
        move.BackwardUntillWall();
        
        // Vyložení první
        if (first.color == second.color) {
            logMsg("Přesouvám obě ramena dozadu (stejná barva).");
            arm.SmallerBack();
            arm.BiggerBack();
            delay(1000);
            grab.SmallerArmOpen();
            grab.BiggerArmOpen();
            delay(1000);
            logMsg("Vracím obě ramena nahoru.");
            arm.SmallerUp();
            arm.BiggerUp();
            delay(200);
        } else {
            if (first.is_smaller_arm) {
                logMsg("Přesouvám malé rameno dozadu.");
                arm.SmallerBack();
                delay(1000);
                grab.SmallerArmOpen();
                delay(1000);
                logMsg("Vracím malé rameno nahoru.");
                arm.SmallerUp();
                delay(200);
            } else {
                logMsg("Přesouvám velké rameno dozadu.");
                arm.BiggerBack();
                delay(1000);
                grab.BiggerArmOpen();
                delay(1000);
                logMsg("Vracím velké rameno nahoru.");
                arm.BiggerUp();
                delay(200);
            }
        }
        
        move.Straight(2000, 100, 1000);
        
        // Aktualizace is_left_wall podle toho, kde jsme vykládali
        if (first.color == COLOR_RED || first.color == COLOR_GREEN) {
            is_left_wall = true;
        } else {
            is_left_wall = false;
        }

        // 2. Doručení (vzdálenější kostka) - pouze pokud se liší barvy a máme druhou kostku
        if (second.color != NIC && second.color != first.color) {
            const char* color_name2 = (second.color == COLOR_RED) ? "ČERVENOU" : ((second.color == COLOR_GREEN) ? "ZELENOU" : "MODROU");
            logMsg("Doručuji kostky: Přejiždím na %s pozici (2. krok).", color_name2);
            
            int dist_diff = second.distance - first.distance;
            if (dist_diff < 0) dist_diff = 0;
            
            // Otočení rovnoběžně se stěnou (směrem od zadní stěny)
            is_left_wall ? move.TurnLeft(90) : move.TurnRight(90);
            
            move.Straight(2500, dist_diff, 6000);
            move.Stop();
            
            // Otočení čelem ke stěně s barevnými zónami
            is_left_wall ? move.TurnRight(90) : move.TurnLeft(90);
            move.BackwardUntillWall();
            
            // Vyložení druhé
            if (second.is_smaller_arm) {
                logMsg("Přesouvám malé rameno dozadu.");
                arm.SmallerBack();
                delay(1000);
                grab.SmallerArmOpen();
                delay(1000);
                logMsg("Vracím malé rameno nahoru.");
                arm.SmallerUp();
                delay(200);
            } else {
                logMsg("Přesouvám velké rameno dozadu.");
                arm.BiggerBack();
                delay(1000);
                grab.BiggerArmOpen();
                delay(1000);
                logMsg("Vracím velké rameno nahoru.");
                arm.BiggerUp();
                delay(200);
            }
            
            move.Straight(2000, 100, 1000);
            
            // Aktualizace is_left_wall podle druhé barvy
            if (second.color == COLOR_RED || second.color == COLOR_GREEN) {
                is_left_wall = true;
            } else {
                is_left_wall = false;
            }
        }
    }
    
    // Globální proměnné pro zbytek kódu, pokud se to někam posílá
    bliz_leva = is_left_wall;
    bliz_prava = !is_left_wall;
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

  servoBus.begin(2, &man.smartServoBusBackend()); // Inicializace sběrnice pro serva přes STM koprocesor
  grab.Init(); // Inicializace smart serv (limity, AutoStop parametry)
  
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


  for (size_t lap = 0; (lap < 9) && (millis()/1000 - start_time < final_time); lap++)
  {
    logMsg("\n--- Start kola %d ---", lap + 1);
    logMsg("Jedu dopředu pro vyhledání kostek...");
    move.Acceleration(500, 20000, 210);
    move.Straight(32000, 800 - lap*120, 10000); // Příkaz pro pohyb robota rovně na 1 metr s rychlostí 2500 a timeoutem 5 sekund
    move.Stop();
    
    logMsg("Otáčím se k nakládací rampě/stěně.");
    move.TurnLeft(90); // Otoč robota doprava o 90 stupňů
    
    logMsg("Sklápím obě ramena dopředu.");
    arm.SmallerFront();
    arm.BiggerFront();
    
    logMsg("Najíždím ke kostkám a zastavuji před nimi.");
    move.BackwardUntillWall();
    move.Straight(5000, 300, 3000);
    DriveToBricksWithSensors(2500, 550, 6000);
    
    logMsg("Zavírám malé i velké klepeto pro první uchopení.");
    grab.SmallerArmClose();
    grab.BiggerArmClose();
    delay(1000);
    
    logMsg("Couvám ke stěně a otevírám klepeta.");
    move.BackwardUntillWall();
    grab.SmallerArmOpen();
    grab.BiggerArmOpen();
    
    logMsg("Najíždím kousek dopředu a definitivně zavírám klepeta pro uchopení kostek.");
    move.Straight(2000, 150, 2000);
    grab.SmallerArmClose();
    grab.BiggerArmClose();
    delay(200);
    
    Color smaller_arm_brick = sens.GetColorRGB2();
    Color bigger_arm_brick = sens.GetColorRGB1();
    const char* s_color = (smaller_arm_brick == COLOR_RED) ? "červená" : ((smaller_arm_brick == COLOR_GREEN) ? "zelená" : "modrá");
    const char* b_color = (bigger_arm_brick == COLOR_RED) ? "červená" : ((bigger_arm_brick == COLOR_GREEN) ? "zelená" : "modrá");
    logMsg("V malém klepetu jsem chytil %s barvu.", s_color);
    logMsg("V dlouhém (velkém) klepetu jsem chytil %s barvu.", b_color);
    
    BrickDeliver(smaller_arm_brick, bigger_arm_brick, lap);
    grab.SmallerArmOpen();
    grab.BiggerArmOpen();
    
    move.Stop();
    move.TurnRight(90);
    move.BackwardUntillWall();
  }
  logMsg("Konec hlavní smyčky. Zvedám ramena a jedu domů.");
  arm.BiggerUp();
  arm.SmallerUp();
  move.Straight(1000, 40, 1000);
  move.TurnLeft(90);
  move.Acceleration(500, 30000, 300);
  move.Straight(32000, 1700, 10000); // 300 + 2200 = 2.5 metru popředu
  move.TurnRight(180);
  move.BackwardUntillWall(10000);
  move.Straight(1000, 140, 1000);
  move.TurnLeft(88);
  move.BackwardUntillWall(3000);
  move.Acceleration(300, 10000, 250);
  move.ArcRight(90, 190);
  
  move.Acceleration(300, 10000, 120);
  move.ArcLeft(176, 180);
  move.Straight(4000, 10000, 32000);
  logMsg("Robot se vrátil domů. Konec programu.");

}

void loop() {

}

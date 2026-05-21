#include "RBCX.h"
#include <Arduino.h>
#include "Adafruit_TCS34725.h"
#include "gyro.hpp"

struct Grabber;
extern Grabber grab;

typedef enum {
        COLOR_RED,
        COLOR_GREEN,
        COLOR_BLUE,
        NIC
    } Color;

struct Sensors{

    static const uint8_t RGB_SDA_front_pin = 21;
    static const uint8_t RGB_SCL_front_pin = 22;

    static const uint8_t RGB_SDA_down_pin = 14; // *************************************************
    static const uint8_t RGB_SCL_down_pin = 26; 

    Adafruit_TCS34725 rgb_front = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X);
    Adafruit_TCS34725 rgb_down = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X);

    float r_1 = 0, g_1 = 0, b_1 = 0; // barvy na barevnem senzoru 1
    float r_2 = 0, g_2 = 0, b_2 = 0; // barvy na barevnem senzoru 2

    uint16_t red_1 = 0, green_1 = 0, blue_1 = 0, clear_1 = 0; // barvy na barevnem senzoru 1
    uint16_t red_2 = 0, green_2 = 0, blue_2 = 0, clear_2 = 0; // barvy na barevnem senzoru 2

    void InitRGB(){
        delay(100); // Počkej na stabilizaci napájení senzorů
        pinMode(RGB_SDA_front_pin, INPUT_PULLUP);
        pinMode(RGB_SCL_front_pin, INPUT_PULLUP);
        pinMode(RGB_SDA_down_pin, INPUT_PULLUP);
        pinMode(RGB_SCL_down_pin, INPUT_PULLUP);
        Wire1.begin(RGB_SDA_front_pin, RGB_SCL_front_pin, 100000); // pro predni senzor 
        Wire.begin(RGB_SDA_down_pin, RGB_SCL_down_pin, 100000); // pro spodni senzor 

        if (!rgb_front.begin(TCS34725_ADDRESS, &Wire1)) {
            Serial.printf("Can not connect to the front RGB sensor");
            delay(500);  // aby se zprava urcite stihla vypsat 
            abort();
        }

        if (!rgb_down.begin(TCS34725_ADDRESS, &Wire)) {
            Serial.printf("Can not connect to the down RGB sensor");
            delay(500); 
        }
    }

    void ReadRGB(){
        rgb_front.getRGB(&r_1, &g_1, &b_1);
        rgb_down.getRGB(&r_2, &g_2, &b_2);
        rgb_front.getRawData(&red_1, &green_1, &blue_1, &clear_1);
        rgb_down.getRawData(&red_2, &green_2, &blue_2, &clear_2);
    }

    void PrintRGBToSerial(){
        ReadRGB();
        Serial.printf("Front RGB: R: %f, G: %f, B: %f, Clear: %d\n", r_1, g_1, b_1, clear_1);
        Serial.printf("Down RGB: R: %f, G: %f, B: %f, Clear: %d\n", r_2, g_2, b_2, clear_2);
    }

    bool IsBrickRGB1(){
        ReadRGB();
        
        if (clear_1 < 1000){
            return true;
        }
        else{
            return false;
        }
    }

    bool IsBrickRGB2(){
        ReadRGB();
        
        if (clear_2 < 1000){
            return true;
        }
        else{
            return false;
        }
    }

    Color last_color_1 = NIC;
    Color last_color_2 = NIC;

    void SetLed(Color col, bool on) {
        auto& leds = rb::Manager::get().leds();
        if (col == COLOR_RED) leds.red(on);
        else if (col == COLOR_GREEN) leds.green(on);
        else if (col == COLOR_BLUE) leds.blue(on);
    }

    void UpdateLeds() {
        auto& leds = rb::Manager::get().leds();
        
        // Zhasnout všechny LED na začátku
        leds.red(false);
        leds.green(false);
        leds.blue(false);
        leds.yellow(false);

        // Pokud nemáme obě kostky
        if (last_color_1 == NIC || last_color_2 == NIC) {
            // Rozsvítíme pouze ty barvy, které reálně máme
            if (last_color_1 != NIC) {
                SetLed(last_color_1, true);
            }
            if (last_color_2 != NIC) {
                SetLed(last_color_2, true);
            }
            return;
        }

        // Máme obě kostky
        if (last_color_1 != last_color_2) {
            // Různé barvy -> rozsvítit obě
            SetLed(last_color_1, true);
            SetLed(last_color_2, true);
        } else {
            // Stejné barvy -> rozsvítit barvu a žlutou
            SetLed(last_color_1, true);
            leds.yellow(true);
        }
    }

    Color GetColorRGB1() {
        ReadRGB();
        if(!grab.je_kostka_v_klepete(1)){
            last_color_1 = NIC;
            UpdateLeds();
            return NIC;
        }

        Color result = NIC;
        if (r_1 > g_1 && r_1 > b_1) {
            result = COLOR_RED;
        } else if (b_1 > g_1 * 0.80f) {
            result = COLOR_BLUE;
        } else {
            result = COLOR_GREEN;
        }

        last_color_1 = result;
        UpdateLeds();
        return result;
    }

    Color GetColorRGB2() {
        ReadRGB();

        if(!grab.je_kostka_v_klepete(0)){
            last_color_2 = NIC;
            UpdateLeds();
            return NIC;
        }

        Color result = NIC;
        if (r_2 > g_2 && r_2 > b_2) {
            result = COLOR_RED;
        } else if (b_2 > g_2 * 0.80f) {
            result = COLOR_BLUE;
        } else {
            result = COLOR_GREEN;
        }

        last_color_2 = result;
        UpdateLeds();
        return result;
    }
};
#include "RBCX.h"
#include <Arduino.h>
#include "Adafruit_TCS34725.h"

typedef enum {
        COLOR_RED,
        COLOR_GREEN,
        COLOR_BLUE,
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

    Color GetColorRGB1() {
        ReadRGB();

        if (r_1 > g_1 && r_1 > b_1) {
            return COLOR_RED;
        } else if (b_1 > g_1 * 0.80f) {
            return COLOR_BLUE;
        } else {
            return COLOR_GREEN;
        }
    }

    Color GetColorRGB2() {
        ReadRGB();

        if (r_2 > g_2 && r_2 > b_2) {
            return COLOR_RED;
        } else if (b_2 > g_2 * 0.80f) {
            return COLOR_BLUE;
        } else {
            return COLOR_GREEN;
        }
    }
};
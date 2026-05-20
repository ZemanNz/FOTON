#include <Arduino.h>
#include "RBCX.h"

#define GYRO_UART_RX 16
#define GYRO_UART_TX 17
#define GYRO_UART_BAUD 115200

struct __attribute__((__packed__)) GyroPacket {
    uint8_t sync1; // 0x12
    uint8_t sync2; // 0x34
    float angleZ;
};

void setup() {
    // Inicializace RBCX manageru (předchází restartům z důvodu Watchdogu)
    auto &man = rb::Manager::get();
    man.install();

    Serial.begin(115200);
    delay(1000);
    Serial.println("=== RBCX UART GYRO TEST (WITH RBCX MANAGER) ===");

    // Inicializace UART1 pro komunikaci s gyroskopem (ESP32)
    Serial1.begin(GYRO_UART_BAUD, SERIAL_8N1, GYRO_UART_RX, GYRO_UART_TX);
    Serial.printf("Serial1 spusten na RX=%d, TX=%d, Baud=%d\n", GYRO_UART_RX, GYRO_UART_TX, GYRO_UART_BAUD);
}

void loop() {
    // Vyčítáme a vypisujeme vše, co přijde
    while (Serial1.available() > 0) {
        // Pokud začíná hlavička 0x12 a máme dostatek bajtů na celý paket
        if (Serial1.peek() == 0x12 && Serial1.available() >= sizeof(GyroPacket)) {
            GyroPacket pkt;
            Serial1.readBytes((uint8_t*)&pkt, sizeof(GyroPacket));
            
            if (pkt.sync2 == 0x34) {
                Serial.printf("[PAKET] Sync: [0x12, 0x34] -> Uhel Z: %.2f deg\n", pkt.angleZ);
            } else {
                Serial.printf("[CHYBA SYNC] Nasel jsem 0x12, ale druhy sync je 0x%02X (uhel=%.2f)\n", pkt.sync2, pkt.angleZ);
            }
        } else {
            // Ostatní data vypíšeme jako surové bajty v HEX a DEC
            uint8_t rawByte = Serial1.read();
            Serial.printf("[RAW] 0x%02X (%d)\n", rawByte, rawByte);
        }
    }
    delay(1);
}

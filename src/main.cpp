#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <RBCX.h>

auto &man = rb::Manager::get();

Adafruit_MPU6050 mpu;

// Wire1 pins (front I2C bus where color sensor S1 is connected)
static const uint8_t SDA_PIN = 21;
static const uint8_t SCL_PIN = 22;

float gyroZOffset = 0.0f;
float heading = 0.0f; // Cumulative heading in degrees
unsigned long lastTime = 0;

// Calibrate the gyroscope offset at boot
void calibrateGyro() {
    Serial.println("=============================================");
    Serial.println("Calibrating gyroscope... KEEP ROBOT STILL!");
    Serial.println("=============================================");
    
    float sum = 0.0f;
    const int samples = 200;
    
    for (int i = 0; i < samples; i++) {
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        sum += g.gyro.z;
        delay(10);
    }
    
    gyroZOffset = sum / samples;
    Serial.printf("Calibration completed. Static Z offset: %.5f rad/s\n", gyroZOffset);
    Serial.println("Press LEFT button to reset heading to 0.00 deg.");
    Serial.println("=============================================");
}

void setup() {
    Serial.begin(115200);
    delay(500);

    man.install();

    // Initialize Wire1 (pins 21 and 22)
    Wire1.begin(SDA_PIN, SCL_PIN, 100000);

    Serial.println("Initializing Adafruit MPU-6050...");
    
    bool status = false;
    if (mpu.begin(0x68, &Wire1)) {
        status = true;
    } else if (mpu.begin(0x69, &Wire1)) {
        status = true;
    }

    if (!status) {
        Serial.println("Failed to find MPU-6050 chip! Check connection/wiring!");
        while (1) {
            delay(10);
        }
    }

    // Configure sensor settings
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    delay(100);
    
    calibrateGyro();
    
    lastTime = micros();
}

void loop() {
    unsigned long currentTime = micros();
    // Calculate elapsed time in seconds since the last loop iteration
    float dt = (float)(currentTime - lastTime) / 1000000.0f;
    lastTime = currentTime;

    // Get current sensor readings
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Subtract the calibrated static offset
    float gyroZ_corrected = g.gyro.z - gyroZOffset;

    // Apply a deadband threshold to prevent drift when stationary (0.015 rad/s ~ 0.86 deg/s)
    if (abs(gyroZ_corrected) < 0.015f) {
        gyroZ_corrected = 0.0f;
    }

    // Convert rad/s to deg/s and integrate: angle = velocity * time
    float gyroZ_deg = gyroZ_corrected * (180.0f / PI);
    heading += gyroZ_deg * dt;

    // Print output to serial
    Serial.printf("Rotation angle (Heading): %.2f deg | Raw Z: %.4f rad/s\n", heading, g.gyro.z);

    // Reset heading to 0.00 when LEFT button is pressed
    if (man.buttons().left() == 1) {
        heading = 0.0f;
        Serial.println("\n>>> Heading reset to 0.00 deg! <<<\n");
        delay(300);
    }

    delay(20); // Perform integration updates at ~50 Hz
}

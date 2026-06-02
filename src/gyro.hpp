#ifndef GYRO_HPP
#define GYRO_HPP

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

struct GyroState {
    Adafruit_MPU6050 mpu;
    float angleX = 0.0f;
    float angleY = 0.0f;
    float angleZ = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
    float rawOffsetX = 0.0f;
    float rawOffsetY = 0.0f;
    float rawOffsetZ = 0.0f;
    bool initialized = false;
    volatile bool calibrateRequested = false;
};

// Singleton getter to store state safely in header-only library
inline GyroState& getGyroState() {
    static GyroState state;
    return state;
}

// Background FreeRTOS task for real-time integration
inline void gyroTask(void *pvParameters) {
    auto &state = getGyroState();
    unsigned long lastTime = micros();
    
    while (true) {
        if (state.calibrateRequested) {
            state.calibrateRequested = false;
            float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
            const int samples = 100;
            for (int i = 0; i < samples; i++) {
                sensors_event_t a_c, g_c, temp_c;
                state.mpu.getEvent(&a_c, &g_c, &temp_c);
                sumX += g_c.gyro.x;
                sumY += g_c.gyro.y;
                sumZ += g_c.gyro.z;
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            state.rawOffsetX = sumX / samples;
            state.rawOffsetY = sumY / samples;
            state.rawOffsetZ = sumZ / samples;

            state.angleX = 0.0f;
            state.angleY = 0.0f;
            state.angleZ = 0.0f;
            state.offsetX = 0.0f;
            state.offsetY = 0.0f;
            state.offsetZ = 0.0f;
            
            lastTime = micros();
            continue;
        }

        unsigned long currentTime = micros();
        float dt = (float)(currentTime - lastTime) / 1000000.0f;
        lastTime = currentTime;

        sensors_event_t a, g, temp;
        state.mpu.getEvent(&a, &g, &temp);

        // Correct raw rates using static calibration offsets
        float gx = g.gyro.x - state.rawOffsetX;
        float gy = g.gyro.y - state.rawOffsetY;
        float gz = g.gyro.z - state.rawOffsetZ;

        // Apply a small noise deadband
        if (abs(gx) < 0.015f) gx = 0.0f;
        if (abs(gy) < 0.015f) gy = 0.0f;
        if (abs(gz) < 0.015f) gz = 0.0f;

        // Integrate angular velocity (converted to degrees)
        state.angleX += gx * (180.0f / PI) * dt;
        state.angleY += gy * (180.0f / PI) * dt;
        state.angleZ += gz * (180.0f / PI) * dt;

        vTaskDelay(pdMS_TO_TICKS(5)); // 100 Hz update rate
    }
}

// Initialize and calibrate the gyroscope
inline void gyroInit() {
    auto &state = getGyroState();
    if (state.initialized) return;

    // Initialize Wire with SDA=21 and SCL=22
    Wire.begin(21, 22, 100000);

    bool status = false;
    if (state.mpu.begin(0x68, &Wire)) {
        status = true;
    } else if (state.mpu.begin(0x69, &Wire)) {
        status = true;
    }

    if (!status) {
        Serial.println("Failed to find MPU-6050 chip! Check connection/wiring!");
        return;
    }

    // Configure sensor settings
    state.mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    state.mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    state.mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    delay(100);

    // Calibrate gyro offset (Robot MUST be still)
    Serial.println("=============================================");
    Serial.println("Calibrating MPU-6050... KEEP ROBOT STILL!");
    Serial.println("=============================================");
    
    float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
    const int samples = 200;
    for (int i = 0; i < samples; i++) {
        sensors_event_t a, g, temp;
        state.mpu.getEvent(&a, &g, &temp);
        sumX += g.gyro.x;
        sumY += g.gyro.y;
        sumZ += g.gyro.z;
        delay(10);
    }
    state.rawOffsetX = sumX / samples;
    state.rawOffsetY = sumY / samples;
    state.rawOffsetZ = sumZ / samples;

    state.angleX = 0.0f;
    state.angleY = 0.0f;
    state.angleZ = 0.0f;
    state.offsetX = 0.0f;
    state.offsetY = 0.0f;
    state.offsetZ = 0.0f;

    state.initialized = true;
    Serial.println("MPU-6050 Calibration completed successfully!");
    Serial.println("=============================================");

    // Pin task to Core 1 so it doesn't block Core 0 (main Arduino core)
    xTaskCreatePinnedToCore(
        gyroTask,
        "gyroTask",
        4096,
        NULL,
        1,
        NULL,
        1
    );
}

// Get relative angles in degrees
inline float gyroGetAngleX() {
    auto &state = getGyroState();
    return state.angleX - state.offsetX;
}

inline float gyroGetAngleY() {
    auto &state = getGyroState();
    return state.angleY - state.offsetY;
}

inline float gyroGetAngleZ() {
    auto &state = getGyroState();
    return state.angleZ - state.offsetZ;
}

// Reset relative angles to 0.0
inline void gyroResetX() {
    auto &state = getGyroState();
    state.offsetX = state.angleX;
}

inline void gyroResetY() {
    auto &state = getGyroState();
    state.offsetY = state.angleY;
}

inline void gyroResetZ() {
    auto &state = getGyroState();
    state.offsetZ = state.angleZ;
}

inline void gyroRequestCalibration() {
    auto &state = getGyroState();
    state.calibrateRequested = true;
}

inline void gyroResetAll() {
    gyroResetX();
    gyroResetY();
    gyroResetZ();
}

#endif // GYRO_HPP

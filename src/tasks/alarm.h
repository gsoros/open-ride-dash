#ifndef ALARM_H
#define ALARM_H

/*
 * TODO: This is a WIP sketch for a standalone alarm system that uses the MPU6050 to detect motion. It will eventually be turned into a real task.
 */

#include <Arduino.h>
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050.h"

// Externalize global alarm state so the CAN task and BLE task can read/write it
enum AlarmState { DISARMED,
                  ARMED_IDLE,
                  WARNING_CHIRP,
                  LATCHED_ALARM };
extern volatile AlarmState currentAlarmState;

#define MPU_INT_PIN 4
#define BUZZER_PIN 5

// FreeRTOS Handles
TaskHandle_t mpuTaskHandle = NULL;
volatile TaskHandle_t isrTargetTaskHandle = NULL;

// 1. Lightweight, IRAM-safe ISR
void IRAM_ATTR mpuInterruptHandler() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (isrTargetTaskHandle != NULL) {
        // Unblock the MPU task immediately from the interrupt context
        vTaskNotifyGiveFromISR(isrTargetTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR();
    }
}

// Helper to clear the I2C interrupt line outside the ISR
void clearMPUInterrupt() {
    Wire.beginTransmission(0x68);
    Wire.write(0x3A);
    Wire.endTransmission(false);
    Wire.requestFrom(0x68, 1);
    if (Wire.available()) {
        Wire.read();
    }
}

// 2. The Standalone Alarm Task
void vAlarmTask(void* pvParameters) {
    MPU6050 mpu;
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    pinMode(MPU_INT_PIN, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(MPU_INT_PIN), mpuInterruptHandler, RISING);

    // Save this task's handle so the ISR knows who to notify
    isrTargetTaskHandle = xTaskGetCurrentTaskHandle();

    // Configure MPU Wake-on-Motion (safe to use Wire here)
    mpu.initialize();
    mpu.setSleepEnabled(false);
    mpu.setDHPFMode(MPU6050_DHPF_5);
    mpu.setMotionDetectionThreshold(15);
    mpu.setMotionDetectionDuration(2);
    mpu.setIntMotionEnabled(true);
    clearMPUInterrupt();

    unsigned long stateTimer = 0;
    unsigned long buzzerToggleTimer = 0;
    bool buzzerStatus = false;

    while (1) {
        // If we are idle, block indefinitely until the ISR sends a notification
        if (currentAlarmState == ARMED_IDLE) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            // Motion detected! Clear the interrupt register immediately over I2C
            clearMPUInterrupt();
            currentAlarmState = WARNING_CHIRP;
            stateTimer = millis();
            buzzerToggleTimer = millis();
            digitalWrite(BUZZER_PIN, HIGH);
        }

        // If we are actively processing alarms, poll using short non-blocking ticks
        if (currentAlarmState == WARNING_CHIRP) {
            unsigned long currentMillis = millis();

            // Check if another interrupt arrived during the warning period
            if (ulTaskNotifyTake(pdTRUE, 0) == pdTRUE) {
                clearMPUInterrupt();
                currentAlarmState = LATCHED_ALARM;
                stateTimer = currentMillis;
            } else if (currentMillis - stateTimer >= 1500) {
                // Warning timed out with no further motion
                digitalWrite(BUZZER_PIN, LOW);
                clearMPUInterrupt();
                currentAlarmState = ARMED_IDLE;
            }

            vTaskDelay(pdMS_TO_TICKS(20));  // Short evaluation delay
        }

        if (currentAlarmState == LATCHED_ALARM) {
            unsigned long currentMillis = millis();

            // Toggle buzzer every 100ms
            if (currentMillis - buzzerToggleTimer >= 100) {
                buzzerToggleTimer = currentMillis;
                buzzerStatus = !buzzerStatus;
                digitalWrite(BUZZER_PIN, buzzerStatus);
            }

            // Keep the hardware buffer clear so it doesn't lock up
            if (ulTaskNotifyTake(pdTRUE, 0) == pdTRUE) {
                clearMPUInterrupt();
            }

            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

#endif  // ALARM_H
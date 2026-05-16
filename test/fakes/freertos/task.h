#pragma once

#include "FreeRTOS.h"

using TaskFunction_t = void (*)(void*);

BaseType_t xTaskCreate(TaskFunction_t taskCode,
                       const char* name,
                       uint32_t stackDepth,
                       void* parameters,
                       UBaseType_t priority,
                       TaskHandle_t* createdTask);

TaskHandle_t xTaskGetCurrentTaskHandle();
TickType_t xTaskGetTickCount();
void vTaskDelay(TickType_t ticks);
void vTaskDelayUntil(TickType_t* previousWakeTime, TickType_t increment);
void vTaskDelete(TaskHandle_t taskToDelete);
void vTaskSuspend(TaskHandle_t taskToSuspend);
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t task);
uint32_t ulTaskNotifyTake(BaseType_t clearCountOnExit, TickType_t ticksToWait);
BaseType_t xTaskNotifyGive(TaskHandle_t taskToNotify);


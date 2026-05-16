#pragma once

#include <cstdint>

using TickType_t = uint32_t;
using UBaseType_t = uint32_t;
using BaseType_t = int;
using StackType_t = uint32_t;
using TaskHandle_t = void*;

struct portMUX_TYPE {
    int locked;
};

#define portMUX_INITIALIZER_UNLOCKED \
    { 0 }

#define pdTRUE 1
#define pdPASS 1
#define portMAX_DELAY UINT32_MAX
#define configMINIMAL_STACK_SIZE 128
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))

void portENTER_CRITICAL(portMUX_TYPE* mux);
void portEXIT_CRITICAL(portMUX_TYPE* mux);


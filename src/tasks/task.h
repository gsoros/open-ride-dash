#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class Task {
   public:
    virtual void taskSetup() {
        taskSetupDone = true;
    };

    virtual void taskRun() {};

    virtual const char* taskName() = 0;

    /*
     * Start the task with the given frequency (Hz), stack size (bytes), and priority.
     * - `frequency`: behavior selection:
     *     - `> 0`: call `taskRun()` periodically at this frequency (Hz).
     *     - `== 0`: never call `taskRun()` (task will suspend after `taskSetup()`).
     *     - `< 0`: call `taskRun()` continuously with a minimal yield.
     * - `stack`: requested stack size in bytes. If 0, uses `configMINIMAL_STACK_SIZE`.
     * - `priority`: FreeRTOS priority. If -1, defaults to 1.
     */
    virtual bool taskStart(float frequency = -1,
                           uint32_t stack = 0,
                           int8_t priority = -1) {
        ESP_LOGD(taskName(), "Starting task with frequency %.2f Hz, stack %u bytes, priority %d",
                 frequency, stack, priority);
        if (taskHandle != nullptr) {
            ESP_LOGE(taskName(), "Already running");
            return false;
        }

        taskWriteFrequency(frequency);

        // Convert stack bytes to words (FreeRTOS stack depth is in words)
        uint32_t stackWords = 0;
        if (stack == 0) {
            stackWords = configMINIMAL_STACK_SIZE;
        } else {
            stackWords = stack / sizeof(StackType_t);
            if (stackWords == 0) stackWords = 1;
        }

        UBaseType_t prio = (priority < 0) ? 1 : (UBaseType_t)priority;

        BaseType_t res = xTaskCreate(
            &Task::taskTrampoline,  // task entry
            taskName(),             // name
            stackWords,             // stack depth (words)
            this,                   // params
            prio,                   // priority
            &taskHandle             // handle
        );

        if (res != pdPASS) {
            taskHandle = nullptr;
            ESP_LOGE(taskName(), "Failed to create task (res=%d)", res);
            return false;
        }
        return true;
    };

    virtual float taskGetFrequency() const {
        return taskReadFrequency();
    }

    virtual void taskSetFrequency(float frequency) {
        ESP_LOGD(taskName(), "Setting frequency to %.2f Hz", frequency);
        taskWriteFrequency(frequency);
        TaskHandle_t h = taskHandle;
        if (h != nullptr) {
            xTaskNotifyGive(h);
        }
    }

    virtual void taskStop() {
        if (taskHandle == nullptr) return;
        TaskHandle_t h = taskHandle;
        taskHandle = nullptr;
        vTaskDelete(h);
    };

    Task() {};
    virtual ~Task() {
        taskStop();
    };

    /*
     * Return lowest free stack (high water mark) in bytes, or -1 if unavailable.
     */
    virtual int taskGetLowestStackLevel() {
        if (taskHandle == nullptr) return -1;
#if defined(configUSE_TRACE_FACILITY) || defined(configUSE_STATS_FORMATTING_FUNCTIONS)
        UBaseType_t high = uxTaskGetStackHighWaterMark(taskHandle);
        return (int)(high * sizeof(StackType_t));
#else
        UBaseType_t high = uxTaskGetStackHighWaterMark(taskHandle);
        return (int)high;  // best-effort: returns words if bytes not available
#endif
    };

   protected:
    TaskHandle_t taskHandle = nullptr;
    float taskFrequencyHz = -1;
    portMUX_TYPE taskFrequencyMux = portMUX_INITIALIZER_UNLOCKED;
    bool taskSetupDone = false;

    float taskReadFrequency() const {
        portMUX_TYPE* mux = const_cast<portMUX_TYPE*>(&taskFrequencyMux);
        portENTER_CRITICAL(mux);
        float frequency = taskFrequencyHz;
        portEXIT_CRITICAL(mux);
        return frequency;
    }

    void taskWriteFrequency(float frequency) {
        portENTER_CRITICAL(&taskFrequencyMux);
        taskFrequencyHz = frequency;
        portEXIT_CRITICAL(&taskFrequencyMux);
    }

    static TickType_t taskFrequencyToTicks(float frequency) {
        TickType_t ticks = pdMS_TO_TICKS((uint32_t)(1000.0f / frequency));
        return (ticks == 0) ? 1 : ticks;
    }

    static void taskTrampoline(void* pvParameters) {
        Task* self = static_cast<Task*>(pvParameters);
        if (self == nullptr) {
            vTaskDelete(nullptr);
            return;
        }

        // Set the handle for this task
        self->taskHandle = xTaskGetCurrentTaskHandle();

        if (!self->taskSetupDone) {
            ESP_LOGW(self->taskName(), "Setup has been skipped");
        }

        for (;;) {
            float frequency = self->taskReadFrequency();
            if (frequency == 0.0f) {
                // Do not run until taskSetFrequency() wakes the task with a new mode.
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            } else if (frequency > 0.0f) {
                self->taskRun();
                ulTaskNotifyTake(pdTRUE, taskFrequencyToTicks(frequency));
            } else {
                // frequency < 0: run continuously with minimal yield
                self->taskRun();
                ulTaskNotifyTake(pdTRUE, 1);
            }
        }
    }
};

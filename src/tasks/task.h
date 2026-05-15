#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class Task {
   public:
    virtual void setup() {};
    virtual void run() {};

    virtual const char* taskName() = 0;

    /*
     * Start the task with the given frequency (Hz), stack size (bytes), and priority.
     * - `frequency`: behavior selection:
     *     - `> 0`: call `run()` periodically at this frequency (Hz).
     *     - `== 0`: never call `run()` (task will suspend after `setup()`).
     *     - `< 0`: call `run()` continuously with a minimal yield.
     * - `stack`: requested stack size in bytes. If 0, uses `configMINIMAL_STACK_SIZE`.
     * - `priority`: FreeRTOS priority. If -1, defaults to 1.
     */
    virtual void taskStart(float frequency = -1,
                           uint32_t stack = 0,
                           int8_t priority = -1) {
        if (taskHandle != nullptr) return;  // already running

        taskFrequencyHz = frequency;

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

        (void)res;  // ignore result for now; taskHandle will be nullptr on failure
    };

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

    static void taskTrampoline(void* pvParameters) {
        Task* self = static_cast<Task*>(pvParameters);
        if (self == nullptr) {
            vTaskDelete(nullptr);
            return;
        }

        // Set the handle for this task
        self->taskHandle = xTaskGetCurrentTaskHandle();

        // Call setup once
        self->setup();

        if (self->taskFrequencyHz == 0.0f) {
            // Do not run; suspend this task indefinitely. taskStop() may delete it later.
            vTaskSuspend(nullptr);
            // If the task resumes for any reason, delete it to avoid falling through.
            vTaskDelete(nullptr);
            return;
        } else if (self->taskFrequencyHz > 0.0f) {
            const TickType_t period = pdMS_TO_TICKS((uint32_t)(1000.0f / self->taskFrequencyHz));
            TickType_t lastWake = xTaskGetTickCount();
            for (;;) {
                self->run();
                vTaskDelayUntil(&lastWake, period);
            }
        } else {
            // frequency < 0: run continuously with minimal yield
            for (;;) {
                self->run();
                vTaskDelay(1);  // yield to other tasks
            }
        }
    }
};
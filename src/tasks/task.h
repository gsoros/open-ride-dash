#ifndef TASK_H
#define TASK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <atomic>

class Task {
   public:
    virtual void taskSetup() {};
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
    virtual bool taskStart(float frequency = -1, uint32_t stack = 0, int8_t priority = -1) {
        ESP_LOGD(taskName(), "Starting task with frequency %.2f Hz, stack %u bytes, priority %d",
                 frequency, stack, priority);

        if (_taskHandle.load(std::memory_order_relaxed) != nullptr) {
            ESP_LOGE(taskName(), "Already running");
            return false;
        }

        _taskWriteFrequency(frequency);

        // Convert stack bytes to words (FreeRTOS stack depth is in words)
        uint32_t stackWords = (stack == 0) ? configMINIMAL_STACK_SIZE : (stack / sizeof(StackType_t));
        if (stackWords == 0) stackWords = 1;

        UBaseType_t prio = (priority < 0) ? 1 : (UBaseType_t)priority;

        _taskRunning.store(true, std::memory_order_release);

        // Temporary local handle, as C API doesn't know about std::atomic
        TaskHandle_t tmpHandle = nullptr;
        BaseType_t res = xTaskCreate(
            &Task::_taskTrampoline,
            taskName(),
            stackWords,
            this,
            prio,
            &tmpHandle);

        if (res != pdPASS) {
            _taskRunning.store(false, std::memory_order_relaxed);
            ESP_LOGE(taskName(), "Failed to create task (res=%d)", res);
            return false;
        }

        _taskHandle.store(tmpHandle, std::memory_order_release);
        _taskIsSuspended.store(false, std::memory_order_relaxed);
        return true;
    }

    virtual float taskGetFrequency() const {
        return _taskReadFrequency();
    }

    virtual void taskSetFrequency(float frequency) {
        ESP_LOGD(taskName(), "Setting frequency to %.2f Hz", frequency);
        _taskWriteFrequency(frequency);

        TaskHandle_t h = _taskHandle.load(std::memory_order_acquire);
        if (h != nullptr) {
            xTaskNotifyGive(h);  // Wake up the task if it's sleeping
        }
    }

    virtual void taskSuspend() {
        TaskHandle_t h = _taskHandle.load(std::memory_order_acquire);
        if (h == nullptr || _taskIsSuspended.load(std::memory_order_relaxed)) return;

        ESP_LOGD(taskName(), "Suspending task");
        _taskIsSuspended.store(true, std::memory_order_relaxed);
        vTaskSuspend(h);
    }

    virtual void taskResume() {
        TaskHandle_t h = _taskHandle.load(std::memory_order_acquire);
        if (h == nullptr || !_taskIsSuspended.load(std::memory_order_relaxed)) return;

        ESP_LOGD(taskName(), "Resuming task");
        _taskIsSuspended.store(false, std::memory_order_relaxed);
        vTaskResume(h);
    }

    virtual bool taskIsSuspended() const {
        return _taskIsSuspended.load(std::memory_order_relaxed);
    }

    virtual bool taskIsRunning() const {
        return _taskHandle.load(std::memory_order_acquire) != nullptr;
    }

    /*
     * Cooperative stopping: signals the task to stop, wakes it up, and waits for it to finish.
     */
    virtual void taskStop() {
        TaskHandle_t h = _taskHandle.load(std::memory_order_acquire);
        if (h == nullptr) return;

        ESP_LOGD(taskName(), "Stopping task cleanly...");
        _taskRunning.store(false, std::memory_order_release);
        _taskIsSuspended.store(false, std::memory_order_relaxed);

        // If the task is suspended, it needs to be resumed to receive the stop signal
        vTaskResume(h);
        xTaskNotifyGive(h);

        // Active polling until the task handle is nulled out on exit
        while (_taskHandle.load(std::memory_order_acquire) != nullptr) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        ESP_LOGD(taskName(), "Task stopped cleanly.");
    }

    Task() = default;

    virtual ~Task() {
        taskStop();
    }

    virtual int taskGetLowestStackLevel() {
        TaskHandle_t h = _taskHandle.load(std::memory_order_acquire);
        if (h == nullptr) return -1;
        UBaseType_t high = uxTaskGetStackHighWaterMark(h);
#if defined(configUSE_TRACE_FACILITY) || defined(configUSE_STATS_FORMATTING_FUNCTIONS)
        return (int)(high * sizeof(StackType_t));
#else
        return (int)high;
#endif
    }

   protected:
    // Atomics are used to ensure thread-safety
    std::atomic<TaskHandle_t> _taskHandle{nullptr};
    std::atomic<bool> _taskRunning{false};
    std::atomic<bool> _taskIsSuspended{false};

    float _taskFrequencyHz = -1;
    mutable portMUX_TYPE _taskFrequencyMux = portMUX_INITIALIZER_UNLOCKED;

    float _taskReadFrequency() const {
        portENTER_CRITICAL(&_taskFrequencyMux);
        float frequency = _taskFrequencyHz;
        portEXIT_CRITICAL(&_taskFrequencyMux);
        return frequency;
    }

    void _taskWriteFrequency(float frequency) {
        portENTER_CRITICAL(&_taskFrequencyMux);
        _taskFrequencyHz = frequency;
        portEXIT_CRITICAL(&_taskFrequencyMux);
    }

    static TickType_t _taskFrequencyToTicks(float frequency) {
        TickType_t ticks = pdMS_TO_TICKS((uint32_t)(1000.0f / frequency));
        return (ticks == 0) ? 1 : ticks;
    }

    static void _taskTrampoline(void* pvParameters) {
        Task* self = static_cast<Task*>(pvParameters);
        if (self == nullptr) {
            vTaskDelete(nullptr);
            return;
        }

        // We could call self->taskSetup() here, but sometimes it's necessary to have control
        // over whether and when setup() runs (tasks with shared resources, etc.), so we'll leave
        // it to the user to call it.

        // Main loop
        while (self->_taskRunning.load(std::memory_order_acquire)) {
            float frequency = self->_taskReadFrequency();

            if (frequency == 0.0f) {
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            } else if (frequency > 0.0f) {
                self->taskRun();
                ulTaskNotifyTake(pdTRUE, _taskFrequencyToTicks(frequency));
            } else {
                self->taskRun();
                ulTaskNotifyTake(pdTRUE, 1);
            }
        }

        // Clean exit: the task destroys itself after finishing
        self->_taskHandle.store(nullptr, std::memory_order_release);
        vTaskDelete(nullptr);
    }
};

#endif  // TASK_H

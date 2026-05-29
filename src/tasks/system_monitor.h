#include "task.h"
#include "model/state.h"

extern State state;

class SystemMonitor : public Task {
   public:
    virtual const char* taskName() override {
        return "SysMon";
    }

    virtual void setup(Task* const* tasksToMonitor = nullptr, size_t tasksToMonitorCount = 0) {
        tasks = tasksToMonitor;
        taskCount = tasksToMonitorCount;
        Task::taskSetup();
    }

    virtual void taskRun() override {
        uint32_t t = millis();
        static uint32_t lastMonitor = 0;
        if (t - lastMonitor >= 10000) {
            lastMonitor = t;
            for (size_t i = 0; i < taskCount; ++i) {
                Task* task = tasks[i];
                if (task == nullptr) continue;
                ESP_LOGI(taskName(), "%s: %.2f Hz, stack: %d",
                         task->taskName(),
                         task->taskGetFrequency(),
                         task->taskGetLowestStackLevel());
            }
        }

        static uint32_t lastState = 0;
        if (t - lastState >= 500) {
            lastState = t;
            state.getSnapshot();
            State::Snapshot s = state.getSnapshot();
            ESP_LOGI(taskName(), "speed: %.2f, pas: %d, torque: %u, cadence: %u, wheelSp: %.1f, current: %.1f, voltage: %.1f, motorT: %u, contrT: %uC, wheelMax: %.1f, wheelSi: %u, wheelC: %u}",
                     s.speed(),
                     s.pasLevel,
                     s.torque,
                     s.cadence,
                     s.wheelSpeed_x10 / 10.0f,
                     s.batteryCurrent_x20 / 20.0f,
                     s.batteryVoltage_x100 / 100.0f,
                     s.motorTemp,
                     s.controllerTemp,
                     s.wheelMaxSpeed_x100 / 100.0f,
                     s.wheelSize,
                     s.wheelCircumference);
        }
    }

   protected:
    Task* const* tasks = nullptr;
    size_t taskCount = 0;
};

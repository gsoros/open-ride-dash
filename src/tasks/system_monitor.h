#include "task.h"

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
        for (size_t i = 0; i < taskCount; ++i) {
            Task* task = tasks[i];
            if (task == nullptr) continue;
            ESP_LOGI(taskName(), "%s: %.2f Hz, stack: %d",
                     task->taskName(),
                     task->taskGetFrequency(),
                     task->taskGetLowestStackLevel());
        }
    }

   protected:
    Task* const* tasks = nullptr;
    size_t taskCount = 0;
};

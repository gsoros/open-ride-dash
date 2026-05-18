#include "task.h"

class SystemMonitor : public Task {
   public:
    virtual const char* taskName() override {
        return "SysMon";
    }

    virtual void setup(Task* const* tasksToMonitor = nullptr, size_t tasksToMonitorCount = 0) {
        monitoredTasks = tasksToMonitor;
        monitoredTaskCount = tasksToMonitorCount;
        Task::taskSetup();
    }

    virtual void taskRun() override {
        for (size_t i = 0; i < monitoredTaskCount; ++i) {
            Task* task = monitoredTasks[i];
            if (task == nullptr) continue;
            ESP_LOGI(taskName(), "%s: %.2f Hz, stack: %d",
                     task->taskName(),
                     task->taskGetFrequency(),
                     task->taskGetLowestStackLevel());
        }
    }

   protected:
    Task* const* monitoredTasks = nullptr;
    size_t monitoredTaskCount = 0;
};

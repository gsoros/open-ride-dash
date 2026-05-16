#include <cmath>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "tasks/task.h"

namespace fake_rtos {
struct StopTask : public std::exception {};

struct State {
    TaskFunction_t createdTaskCode = nullptr;
    void* createdTaskParameter = nullptr;
    TaskHandle_t createdHandle = nullptr;
    TaskHandle_t currentHandle = reinterpret_cast<TaskHandle_t>(0x1);
    int createCount = 0;
    uint32_t notificationCount = 0;
    std::vector<TickType_t> waits;
    std::function<void(TickType_t)> onWait;
};

State state;

void reset() {
    state = State{};
}
}  // namespace fake_rtos

void portENTER_CRITICAL(portMUX_TYPE* mux) {
    if (mux != nullptr) mux->locked++;
}

void portEXIT_CRITICAL(portMUX_TYPE* mux) {
    if (mux != nullptr) mux->locked--;
}

BaseType_t xTaskCreate(TaskFunction_t taskCode,
                       const char*,
                       uint32_t,
                       void* parameters,
                       UBaseType_t,
                       TaskHandle_t* createdTask) {
    fake_rtos::state.createCount++;
    fake_rtos::state.createdTaskCode = taskCode;
    fake_rtos::state.createdTaskParameter = parameters;
    fake_rtos::state.createdHandle = fake_rtos::state.currentHandle;
    if (createdTask != nullptr) {
        *createdTask = fake_rtos::state.createdHandle;
    }
    return pdPASS;
}

TaskHandle_t xTaskGetCurrentTaskHandle() {
    return fake_rtos::state.currentHandle;
}

TickType_t xTaskGetTickCount() {
    return 0;
}

void vTaskDelay(TickType_t) {}

void vTaskDelayUntil(TickType_t*, TickType_t) {}

void vTaskDelete(TaskHandle_t) {}

void vTaskSuspend(TaskHandle_t) {}

UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t) {
    return 0;
}

uint32_t ulTaskNotifyTake(BaseType_t clearCountOnExit, TickType_t ticksToWait) {
    fake_rtos::state.waits.push_back(ticksToWait);
    if (fake_rtos::state.onWait) {
        fake_rtos::state.onWait(ticksToWait);
    }

    if (fake_rtos::state.notificationCount == 0) {
        return 0;
    }

    uint32_t count = fake_rtos::state.notificationCount;
    fake_rtos::state.notificationCount = clearCountOnExit ? 0 : fake_rtos::state.notificationCount - 1;
    return count;
}

BaseType_t xTaskNotifyGive(TaskHandle_t) {
    fake_rtos::state.notificationCount++;
    return pdPASS;
}

class CountingTask : public Task {
   public:
    int setupCount = 0;
    int runCount = 0;

    const char* taskName() override {
        return "CountingTask";
    }

    void setup() override {
        setupCount++;
    }

    void run() override {
        runCount++;
    }
};

class TaskHarness : public CountingTask {
   public:
    using Task::taskTrampoline;
};

struct AssertionFailure : public std::exception {
    explicit AssertionFailure(std::string message) : message(std::move(message)) {}
    const char* what() const noexcept override { return message.c_str(); }
    std::string message;
};

void expectTrue(bool condition, const std::string& message) {
    if (!condition) throw AssertionFailure(message);
}

void expectEqual(int expected, int actual, const std::string& message) {
    if (expected != actual) {
        throw AssertionFailure(message + " expected=" + std::to_string(expected) +
                               " actual=" + std::to_string(actual));
    }
}

void expectTick(TickType_t expected, TickType_t actual, const std::string& message) {
    if (expected != actual) {
        throw AssertionFailure(message + " expected=" + std::to_string(expected) +
                               " actual=" + std::to_string(actual));
    }
}

void expectFloat(float expected, float actual, const std::string& message) {
    if (std::fabs(expected - actual) > 0.0001f) {
        throw AssertionFailure(message + " expected=" + std::to_string(expected) +
                               " actual=" + std::to_string(actual));
    }
}

void testSetFrequencyBeforeStartDoesNotNotify() {
    fake_rtos::reset();
    CountingTask task;

    task.taskSetFrequency(25.0f);

    expectFloat(25.0f, task.taskGetFrequency(), "setter should store frequency before start");
    expectEqual(0, fake_rtos::state.notificationCount, "setter should not notify without a task handle");
}

void testStartDoesNotCreateDuplicateTask() {
    fake_rtos::reset();
    CountingTask task;

    task.taskStart(1.0f, 4096, 3);
    task.taskStart(5.0f, 4096, 3);

    expectEqual(1, fake_rtos::state.createCount, "taskStart should ignore an already-running task");
    expectFloat(1.0f, task.taskGetFrequency(), "ignored taskStart should not change frequency");
}

void testPausedTaskDoesNotRunUntilFrequencyChanges() {
    fake_rtos::reset();
    TaskHarness task;
    task.taskSetFrequency(0.0f);

    fake_rtos::state.onWait = [&](TickType_t ticksToWait) {
        if (fake_rtos::state.waits.size() == 1) {
            expectTick(portMAX_DELAY, ticksToWait, "paused task should wait indefinitely");
            expectEqual(1, task.setupCount, "paused task should have run setup once");
            expectEqual(0, task.runCount, "paused task should not call run before resume");
            task.taskSetFrequency(2.0f);
        }
        if (fake_rtos::state.waits.size() == 2) {
            throw fake_rtos::StopTask{};
        }
    };

    try {
        TaskHarness::taskTrampoline(&task);
    } catch (const fake_rtos::StopTask&) {
    }

    expectEqual(1, task.setupCount, "frequency change should not rerun setup");
    expectEqual(1, task.runCount, "task should run after resuming from zero frequency");
    expectTick(500, fake_rtos::state.waits.at(1), "resumed task should use the new period");
}

void testFrequencyChangeWakesSleepingPeriodicTask() {
    fake_rtos::reset();
    TaskHarness task;
    task.taskSetFrequency(1.0f);

    fake_rtos::state.onWait = [&](TickType_t ticksToWait) {
        if (fake_rtos::state.waits.size() == 1) {
            expectTick(1000, ticksToWait, "first wait should use initial frequency");
            expectEqual(1, task.runCount, "task should have run before first wait");
            task.taskSetFrequency(10.0f);
            return;
        }

        expectTick(100, ticksToWait, "second wait should use updated frequency");
        throw fake_rtos::StopTask{};
    };

    try {
        TaskHarness::taskTrampoline(&task);
    } catch (const fake_rtos::StopTask&) {
    }

    expectEqual(1, task.setupCount, "frequency change should not rerun setup");
    expectEqual(2, task.runCount, "notification should wake task for another scheduler pass");
}

void testChangingToZeroStopsFurtherRuns() {
    fake_rtos::reset();
    TaskHarness task;
    task.taskSetFrequency(4.0f);

    fake_rtos::state.onWait = [&](TickType_t ticksToWait) {
        if (fake_rtos::state.waits.size() == 1) {
            expectTick(250, ticksToWait, "first wait should use initial positive frequency");
            task.taskSetFrequency(0.0f);
            return;
        }

        expectTick(portMAX_DELAY, ticksToWait, "zero frequency should wait indefinitely");
        throw fake_rtos::StopTask{};
    };

    try {
        TaskHarness::taskTrampoline(&task);
    } catch (const fake_rtos::StopTask&) {
    }

    expectEqual(1, task.setupCount, "pausing should not rerun setup");
    expectEqual(1, task.runCount, "task should not run again after changing to zero");
}

void testHighFrequencyClampsDelayToOneTick() {
    fake_rtos::reset();
    TaskHarness task;
    task.taskSetFrequency(2000.0f);

    fake_rtos::state.onWait = [](TickType_t ticksToWait) {
        expectTick(1, ticksToWait, "sub-millisecond period should clamp to one tick");
        throw fake_rtos::StopTask{};
    };

    try {
        TaskHarness::taskTrampoline(&task);
    } catch (const fake_rtos::StopTask&) {
    }

    expectEqual(1, task.runCount, "high-frequency task should still run before waiting");
}

void testContinuousModeYieldsOneTick() {
    fake_rtos::reset();
    TaskHarness task;
    task.taskSetFrequency(-1.0f);

    fake_rtos::state.onWait = [](TickType_t ticksToWait) {
        expectTick(1, ticksToWait, "continuous mode should yield for one tick");
        throw fake_rtos::StopTask{};
    };

    try {
        TaskHarness::taskTrampoline(&task);
    } catch (const fake_rtos::StopTask&) {
    }

    expectEqual(1, task.runCount, "continuous task should run before yielding");
}

void runTest(const std::string& name, const std::function<void()>& test) {
    test();
    std::cout << "[PASS] " << name << '\n';
}

int main() {
    try {
        runTest("set frequency before start does not notify", testSetFrequencyBeforeStartDoesNotNotify);
        runTest("start does not create duplicate task", testStartDoesNotCreateDuplicateTask);
        runTest("paused task resumes without run/setup leakage", testPausedTaskDoesNotRunUntilFrequencyChanges);
        runTest("periodic task wakes on frequency change", testFrequencyChangeWakesSleepingPeriodicTask);
        runTest("changing to zero stops further runs", testChangingToZeroStopsFurtherRuns);
        runTest("high positive frequency clamps delay", testHighFrequencyClampsDelayToOneTick);
        runTest("continuous mode yields one tick", testContinuousModeYieldsOneTick);
    } catch (const std::exception& err) {
        std::cerr << "[FAIL] " << err.what() << '\n';
        return 1;
    }

    return 0;
}

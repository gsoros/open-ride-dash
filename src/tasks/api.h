#ifndef API_H
#define API_H

#include <Arduino.h>
#include <functional>
#include <cstring>

#include "task.h"
#include "config.h"

class Api : public Task {
   public:
    static constexpr size_t COMMAND_NAME_SIZE = 32;
    static constexpr size_t REQUEST_COMMAND_LINE_SIZE = 128;
    static constexpr size_t REPLY_ARGS_SIZE = REQUEST_COMMAND_LINE_SIZE;
    static constexpr size_t REPLY_DATA_SIZE = 256;
    static constexpr size_t MAX_COMMANDS = 256;
    static constexpr UBaseType_t REQUEST_QUEUE_LENGTH = 8;
    static constexpr size_t REPLY_SERIAL_LINE_SIZE = 300;

    struct Reply;
    struct Command {
        char command[COMMAND_NAME_SIZE];
        const char* helpText = nullptr;
        std::function<Reply(const char* args)> handler;
    };

    struct Request {
        char commandLine[REQUEST_COMMAND_LINE_SIZE];
        QueueHandle_t replyQueue = nullptr;
    };

    struct Reply {
        enum class Code : uint8_t {
            Success,
            UnknownCommand,
            InvalidArgs,
            ExecutionError,
        };

        static const char* codeToString(Code code);

        char command[COMMAND_NAME_SIZE];
        char args[REPLY_ARGS_SIZE];
        Code code = Code::Success;
        uint8_t data[REPLY_DATA_SIZE];
        size_t length = 0;
    };

    virtual const char* taskName() const override;

    virtual void setup();

    bool registerCommand(const char* command,
                         std::function<Reply(const char* args)> handler,
                         const char* helpText = nullptr);

    // Non-blocking enqueue of a command request. Returns true on success.
    bool queueCommand(const char* commandLine, QueueHandle_t replyQueue = nullptr);

    virtual void taskRun() override;

    static void formatReply(const Reply& reply, char* buf, size_t bufSize);

   protected:
    size_t numCommands = 0;
    Command commands[MAX_COMMANDS];
    QueueHandle_t requestQueue = nullptr;

    Reply handleCommand(const char* input);
    Reply versionCommand(const char* args);
    Reply helpCommand(const char* args);
    Reply restartCommand(const char* args);
    Reply nullpointerCommand(const char* args);
    Reply batteryCapacityCommand(const char* args);
    Api::Reply hostnameCommand(const char* args);

    bool parseUInt16(const char* token, uint16_t* value);

#ifdef FEATURE_SERIAL
    void handleSerialInput();
    void formatReplyToSerial(const Reply& reply);
#endif
};

// Global instance declared in main.cpp
extern Api api;

class ApiClient {
   public:
    virtual ~ApiClient() {}

   protected:
    bool apiClientSetup(const char* logTag, UBaseType_t replyQueueLength = 4) {
        this->logTag = logTag;
        apiReplyQueue = xQueueCreate(replyQueueLength, sizeof(Api::Reply));
        if (apiReplyQueue == nullptr) {
            ESP_LOGE(logTag, "Failed to create API reply queue");
            return false;
        }
        return true;
    }

    bool apiClientQueueCommand(const char* commandLine) {
        // ESP_LOGD(logTag, "Calling api.queueCommand('%s')", commandLine);
        return api.queueCommand(commandLine, apiReplyQueue);
    }

    void apiClientReceiveReplies() {
        if (apiReplyQueue == nullptr) return;

        Api::Reply reply;
        while (xQueueReceive(apiReplyQueue, &reply, 0) == pdTRUE) {
            receiveReply(reply);
        }
    }

    virtual void receiveReply(const Api::Reply& reply) = 0;

    QueueHandle_t apiReplyQueue = nullptr;
    const char* logTag = "ApiClient";
};

#endif  // API_H

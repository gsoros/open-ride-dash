#ifndef API_H
#define API_H

#include "task.h"
#include <functional>
#include <cstring>

class Api : public Task {
   public:
    struct Reply;
    struct Command {
        char command[32];
        std::function<Reply(const char* args)> handler;
    };

    struct Request {
        char commandLine[128];
        QueueHandle_t replyQueue = nullptr;
    };

    enum ErrorCode {
        SUCCESS = 0,
        UNKNOWN_COMMAND = 1,
        INVALID_ARGS = 2,
        EXECUTION_ERROR = 3,
    };

    void errorCodeToString(uint8_t code, char* buffer, size_t bufferSize) {
        const char* str = "Unknown";
        switch (code) {
            case SUCCESS:
                str = "Success";
                break;
            case UNKNOWN_COMMAND:
                str = "Unknown Command";
                break;
            case INVALID_ARGS:
                str = "Invalid Arguments";
                break;
            case EXECUTION_ERROR:
                str = "Execution Error";
                break;
        }
        strncpy(buffer, str, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }

    struct Reply {
        char command[32];
        uint8_t errorCode = ErrorCode::SUCCESS;
        uint8_t data[256];
        size_t length = 0;
    };

    virtual const char* taskName() override {
        return "API";
    }

    virtual void setup() {
        // Create request queue
        requestQueue = xQueueCreate(8, sizeof(Request));
        if (requestQueue == nullptr) {
            ESP_LOGE(taskName(), "Failed to create request queue");
        }
        Task::taskSetup();
    }

    bool registerCommand(const char* command, std::function<Reply(const char* args)> handler) {
        if (numCommands >= 256) {
            ESP_LOGE(taskName(), "Command limit reached while registering: %s", command);
            return false;
        }
        strncpy(commands[numCommands].command, command, sizeof(commands[numCommands].command) - 1);
        commands[numCommands].handler = handler;
        numCommands++;
        ESP_LOGI(taskName(), "Registered command: %s", command);
        return true;
    }

    // Non-blocking enqueue of a command request. Returns true on success.
    bool queueCommand(const char* commandLine, QueueHandle_t replyQueue = nullptr) {
        if (requestQueue == nullptr) return false;
        Request req = {};
        strncpy(req.commandLine, commandLine, sizeof(req.commandLine) - 1);
        req.commandLine[sizeof(req.commandLine) - 1] = '\0';
        req.replyQueue = replyQueue;
        BaseType_t res = xQueueSend(requestQueue, &req, 0);
        if (res != pdTRUE) {
            ESP_LOGW(taskName(), "Request queue full, dropping command: %s", commandLine);
            return false;
        }
        return true;
    }

    virtual void taskRun() override {
        if (requestQueue == nullptr) return;
        // Drain any pending requests without blocking
        Request req;
        while (xQueueReceive(requestQueue, &req, 0) == pdTRUE) {
            Reply reply = handleCommand(req.commandLine);
            // If caller requested a reply, send it back (non-blocking)
            if (req.replyQueue != nullptr) {
                BaseType_t sent = xQueueSend(req.replyQueue, &reply, 0);
                if (sent != pdTRUE) {
                    ESP_LOGW(taskName(), "Failed to enqueue reply for command %s", reply.command);
                }
            }
        }
    }

   protected:
    uint8_t numCommands = 0;
    Command commands[256];
    QueueHandle_t requestQueue = nullptr;

    Reply handleCommand(const char* input) {
        Reply reply = {};
        char cmd[32];
        const char* args = nullptr;
        sscanf(input, "%31s", cmd);   // Extract command (first word)
        args = input + strlen(cmd);   // Point to the rest of the string as arguments
        while (*args == ' ') args++;  // Skip leading spaces
        for (uint8_t i = 0; i < numCommands; ++i) {
            if (strcmp(cmd, commands[i].command) == 0) {
                // Call handler to produce a reply
                reply = commands[i].handler(args);
                // Ensure reply.command contains the command name (caller may not set it)
                strncpy(reply.command, commands[i].command, sizeof(reply.command) - 1);
                reply.command[sizeof(reply.command) - 1] = '\0';
                reply.length = strlen((char*)reply.data);
                ESP_LOGD(taskName(), "Handled command: %s, args: %s, reply length: %d", cmd, args, reply.length);
                return reply;
            }
        }
        reply.errorCode = ErrorCode::UNKNOWN_COMMAND;
        strncpy(reply.command, cmd, sizeof(reply.command) - 1);
        reply.command[sizeof(reply.command) - 1] = '\0';
        snprintf((char*)reply.data, sizeof(reply.data), "Unknown command: %s", cmd);
        reply.length = strlen((char*)reply.data);
        return reply;
    }
};

// Global instance declared in main.cpp
extern Api api;

class ApiClient {
   public:
    virtual ~ApiClient() {}

   protected:
    bool apiClientSetup(const char* logTag, UBaseType_t replyQueueLength = 4) {
        apiReplyQueue = xQueueCreate(replyQueueLength, sizeof(Api::Reply));
        if (apiReplyQueue == nullptr) {
            ESP_LOGE(logTag, "Failed to create API reply queue");
            return false;
        }
        return true;
    }

    bool apiClientQueueCommand(const char* commandLine) {
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
};

#endif  // API_H

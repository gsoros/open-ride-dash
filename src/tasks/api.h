#ifndef API_H
#define API_H

#include "task.h"
#include <functional>
#include <cstring>

class Api : public Task {
   public:
    static constexpr size_t COMMAND_NAME_SIZE = 32;
    static constexpr size_t REQUEST_COMMAND_LINE_SIZE = 128;
    static constexpr size_t REPLY_DATA_SIZE = 256;
    static constexpr size_t MAX_COMMANDS = 256;
    static constexpr UBaseType_t REQUEST_QUEUE_LENGTH = 8;

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
        char command[COMMAND_NAME_SIZE];
        uint8_t errorCode = ErrorCode::SUCCESS;
        uint8_t data[REPLY_DATA_SIZE];
        size_t length = 0;
    };

    virtual const char* taskName() override {
        return "API";
    }

    virtual void setup() {
        // Create request queue
        requestQueue = xQueueCreate(REQUEST_QUEUE_LENGTH, sizeof(Request));
        if (requestQueue == nullptr) {
            ESP_LOGE(taskName(), "Failed to create request queue");
        }
        registerCommand(
            "help",
            [this](const char* args) {
                return helpCommand(args);
            },
            "Usage: help [command]\nLists commands or shows detailed help for one command.");
        Task::taskSetup();
    }

    bool registerCommand(const char* command, std::function<Reply(const char* args)> handler,
                         const char* helpText = nullptr) {
        if (numCommands >= MAX_COMMANDS) {
            ESP_LOGE(taskName(), "Command limit reached while registering: %s", command);
            return false;
        }
        strncpy(commands[numCommands].command, command, sizeof(commands[numCommands].command) - 1);
        commands[numCommands].command[sizeof(commands[numCommands].command) - 1] = '\0';
        commands[numCommands].helpText = helpText;
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
        size_t length = strlen(req.commandLine);
        while (length > 0 && (req.commandLine[length - 1] == '\r' || req.commandLine[length - 1] == '\n')) {
            req.commandLine[--length] = '\0';
        }
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
    size_t numCommands = 0;
    Command commands[MAX_COMMANDS];
    QueueHandle_t requestQueue = nullptr;

    Reply handleCommand(const char* input) {
        Reply reply = {};
        if (input == nullptr) input = "";

        while (*input == ' ' || *input == '\t' || *input == '\r' || *input == '\n') input++;

        char cmd[COMMAND_NAME_SIZE] = {};
        size_t length = 0;
        while (input[length] != '\0' && input[length] != ' ' && input[length] != '\t' && input[length] != '\r' &&
               input[length] != '\n') {
            if (length >= sizeof(cmd) - 1) {
                reply.errorCode = ErrorCode::INVALID_ARGS;
                snprintf((char*)reply.data, sizeof(reply.data), "Command name too long");
                reply.length = strlen((char*)reply.data);
                return reply;
            }
            cmd[length] = input[length];
            ++length;
        }
        if (length == 0) {
            reply.errorCode = ErrorCode::INVALID_ARGS;
            snprintf((char*)reply.data, sizeof(reply.data), "Empty command");
            reply.length = strlen((char*)reply.data);
            return reply;
        }

        const char* args = input + length;
        while (*args == ' ' || *args == '\t') args++;  // Skip leading argument whitespace
        for (size_t i = 0; i < numCommands; ++i) {
            if (strcmp(cmd, commands[i].command) == 0) {
                // Call handler to produce a reply
                reply = commands[i].handler(args);
                // Ensure reply.command contains the command name (handler may not set it)
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

    Reply helpCommand(const char* args) {
        Reply reply = {};
        if (args == nullptr) args = "";
        while (*args == ' ' || *args == '\t' || *args == '\r' || *args == '\n') args++;

        if (*args == '\0') {
            size_t used = snprintf((char*)reply.data, sizeof(reply.data), "Commands:");
            for (size_t i = 0; i < numCommands && used < sizeof(reply.data); ++i) {
                int written = snprintf((char*)reply.data + used, sizeof(reply.data) - used, "%s%s", i == 0 ? " " : ", ",
                                       commands[i].command);
                if (written < 0) break;
                used += (size_t)written;
            }
            reply.data[sizeof(reply.data) - 1] = '\0';
            return reply;
        }

        char command[COMMAND_NAME_SIZE] = {};
        size_t length = 0;
        while (args[length] != '\0' && args[length] != ' ' && args[length] != '\t' && args[length] != '\r' &&
               args[length] != '\n') {
            if (length >= sizeof(command) - 1) {
                reply.errorCode = ErrorCode::INVALID_ARGS;
                snprintf((char*)reply.data, sizeof(reply.data), "Command name too long");
                return reply;
            }
            command[length] = args[length];
            ++length;
        }

        const char* rest = args + length;
        while (*rest == ' ' || *rest == '\t' || *rest == '\r' || *rest == '\n') rest++;
        if (*rest != '\0') {
            reply.errorCode = ErrorCode::INVALID_ARGS;
            snprintf((char*)reply.data, sizeof(reply.data), "Usage: help [command]");
            return reply;
        }

        for (size_t i = 0; i < numCommands; ++i) {
            if (strcmp(command, commands[i].command) == 0) {
                snprintf((char*)reply.data, sizeof(reply.data), "%s: %s", commands[i].command,
                         commands[i].helpText != nullptr ? commands[i].helpText : "No detailed help available.");
                return reply;
            }
        }

        reply.errorCode = ErrorCode::UNKNOWN_COMMAND;
        snprintf((char*)reply.data, sizeof(reply.data), "Unknown command: %s", command);
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

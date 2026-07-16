#include "tasks/api.h"

#include "model/state.h"
extern State state;

#include "util.h"
#include "build_info.h"  // whoami

const char* Api::taskName() const {
    return "API";
}

void Api::setup() {
    requestQueue = xQueueCreate(REQUEST_QUEUE_LENGTH, sizeof(Request));
    if (requestQueue == nullptr) {
        ESP_LOGE(taskName(), "Failed to create request queue");
    }
    registerCommand(
        "v",
        [this](const char* args) {
            return versionCommand(args);
        },
        "Usage: v\nShows the version information.");
    registerCommand(
        "help",
        [this](const char* args) {
            return helpCommand(args);
        },
        "Usage: help[ command]\nLists commands or shows detailed help for one command.");
    registerCommand(
        "restart",
        [this](const char* args) {
            return restartCommand(args);
        },
        "Usage: restart\nRestarts the system.");
    registerCommand(
        "nullpointer",
        [this](const char* args) {
            return nullpointerCommand(args);
        },
        "Usage: nullpointer\nSimulates a null pointer dereference for testing.");
    registerCommand(
        "battery",
        [this](const char* args) {
            return batteryCapacityCommand(args);
        },
        "Usage: battery[ capacity]\nShows or sets the battery capacity in Wh.");
    registerCommand(
        "hostname",
        [this](const char* args) {
            return hostnameCommand(args);
        },
        "Usage: hostname[ hostname]\nShows or stores the hostname.");
}

bool Api::registerCommand(const char* command,
                          std::function<Reply(const char* args)> handler,
                          const char* helpText) {
    if (numCommands >= MAX_COMMANDS) {
        ESP_LOGE(taskName(), "Command limit reached: %s", command);
        return false;
    }
    Util::copyString(commands[numCommands].command, sizeof(commands[numCommands].command), command);
    commands[numCommands].helpText = helpText;
    commands[numCommands].handler = handler;
    numCommands++;
    ESP_LOGI(taskName(), "Registered command: %s", command);
    return true;
}

bool Api::queueCommand(const char* commandLine, QueueHandle_t replyQueue) {
    if (requestQueue == nullptr) {
        ESP_LOGE(taskName(), "Request queue is null");
        return false;
    }
    Request req = {};
    Util::copyString(req.commandLine, sizeof(req.commandLine), commandLine);
    size_t length = strlen(req.commandLine);
    while (length > 0 && (req.commandLine[length - 1] == '\r' || req.commandLine[length - 1] == '\n')) {
        req.commandLine[--length] = '\0';
    }
    req.replyQueue = replyQueue;
    BaseType_t res = xQueueSend(requestQueue, &req, 0);
    if (res != pdTRUE) {
        ESP_LOGW(taskName(), "Request queue full, dropping: %s", commandLine);
        return false;
    }
    return true;
}

void Api::taskRun() {
    if (requestQueue == nullptr) return;
    // Drain any pending requests without blocking
    Request req;
    while (xQueueReceive(requestQueue, &req, 0) == pdTRUE) {
        // ESP_LOGD(taskName(), "Received request: %s", req.commandLine);
        Reply reply = handleCommand(req.commandLine);
        // If caller requested a reply, send it back (non-blocking)
        if (req.replyQueue != nullptr) {
            // ESP_LOGD(taskName(), "Sending reply: (%d) %s", reply.code, reply.data);
            BaseType_t sent = xQueueSend(req.replyQueue, &reply, 0);
            if (sent != pdTRUE) {
                ESP_LOGW(taskName(), "Failed to enqueue reply for: %s", reply.command);
            }
        }
    }

#ifdef FEATURE_SERIAL
    handleSerialInput();
#endif
}

Api::Reply Api::handleCommand(const char* input) {
    Reply reply = {};
    if (input == nullptr) input = "";

    char cmd[COMMAND_NAME_SIZE] = {};
    if (!Util::nextToken(input, cmd, sizeof(cmd))) {
        reply.code = Reply::Code::UnknownCommand;
        Util::copyString(reply.args, sizeof(reply.args), input);
        snprintf((char*)reply.data, sizeof(reply.data),
                 *input != '\0' ? "Command name too long" : "Empty command");
        reply.length = strlen((char*)reply.data);
        return reply;
    }

    const char* args = Util::skipWhitespace(input);
    for (size_t i = 0; i < numCommands; ++i) {
        if (strcmp(cmd, commands[i].command) == 0) {
            // Call handler to produce a reply
            reply = commands[i].handler(args);
            // Ensure reply.command contains the command name, args, and length (handler may not set it)
            Util::copyString(reply.command, sizeof(reply.command), commands[i].command);
            Util::copyString(reply.args, sizeof(reply.args), args);
            reply.length = strlen((char*)reply.data);
            // ESP_LOGD(taskName(), "Handled command: %s, args: %s, reply length: %d", cmd, args, reply.length);
            return reply;
        }
    }
    reply.code = Reply::Code::UnknownCommand;
    Util::copyString(reply.command, sizeof(reply.command), cmd);
    Util::copyString(reply.args, sizeof(reply.args), args);
    snprintf((char*)reply.data, sizeof(reply.data), "'%s' is a mystery", cmd);
    reply.length = strlen((char*)reply.data);
    return reply;
}

Api::Reply Api::versionCommand(const char* args) {
    Reply reply = {};
    snprintf((char*)reply.data, sizeof(reply.data), "%s", whoami);
    reply.length = strlen((char*)reply.data);
    return reply;
}

Api::Reply Api::helpCommand(const char* args) {
    Reply reply = {};
    args = Util::skipWhitespace(args);

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
    if (!Util::nextToken(args, command, sizeof(command))) {
        reply.code = Reply::Code::InvalidArgs;
        snprintf((char*)reply.data, sizeof(reply.data), "Command name too long");
        return reply;
    }

    const char* rest = Util::skipWhitespace(args);
    if (*rest != '\0') {
        reply.code = Reply::Code::InvalidArgs;
        snprintf((char*)reply.data, sizeof(reply.data), "Usage: help[ command]");
        return reply;
    }

    for (size_t i = 0; i < numCommands; ++i) {
        if (strcmp(command, commands[i].command) == 0) {
            snprintf((char*)reply.data, sizeof(reply.data), "%s: %s", commands[i].command,
                     commands[i].helpText != nullptr ? commands[i].helpText : "No detailed help available.");
            return reply;
        }
    }

    reply.code = Reply::Code::UnknownCommand;
    snprintf((char*)reply.data, sizeof(reply.data), "%s", command);
    return reply;
}

Api::Reply Api::restartCommand(const char* args) {
    ESP_LOGI(taskName(), "Restarting system...");
    esp_restart();
    // We should never reach this point, but if we do, return an error reply
    Reply reply = {};
    reply.code = Reply::Code::ExecutionError;
    snprintf((char*)reply.data, sizeof(reply.data), "Failed to restart system");
    return reply;
}

Api::Reply Api::nullpointerCommand(const char* args) {
    ESP_LOGI(taskName(), "Simulating null pointer dereference for testing...");
    int* p = nullptr;
    ESP_LOGI(taskName(), "Dereferencing null pointer, this should cause a crash: %d", *p);
    Reply reply = {};
    reply.code = Reply::Code::ExecutionError;
    snprintf((char*)reply.data, sizeof(reply.data), "Dereferenced null pointer (this should not happen)");
    return reply;
}

Api::Reply Api::batteryCapacityCommand(const char* args) {
    Reply reply = {};
    args = Util::skipWhitespace(args);

    // get batteryCapacity
    if (*args == '\0') {
        snprintf((char*)reply.data, sizeof(reply.data), "%u", state.batteryCapacity());
        return reply;
    }

    // set batteryCapacity
    char token[6] = {};
    if (!Util::nextToken(args, token, sizeof(token))) return reply;

    const char* rest = Util::skipWhitespace(args);
    if (*rest != '\0') return reply;

    uint16_t value = 0;
    if (!parseUInt16(token, &value)) return reply;
    state.batteryCapacity(value);
    snprintf((char*)reply.data, sizeof(reply.data), "Battery capacity set to %u Wh", value);
    return reply;
}

bool Api::parseUInt16(const char* token, uint16_t* value) {
    if (token == nullptr || value == nullptr) return false;
    char* end = nullptr;
    *value = (uint16_t)strtoul(token, &end, 10);
    return end != nullptr && *end == '\0';
}

Api::Reply Api::hostnameCommand(const char* args) {
    Api::Reply reply = {};
    args = Util::skipWhitespace(args);

    // get hostname
    if (*args == '\0') {
        snprintf((char*)reply.data, sizeof(reply.data), "%s", state.hostname());
        return reply;
    }

    // set hostname — copy and trim
    char newValue[32] = {};
    Util::copyString(newValue, sizeof(newValue), args);
    Util::trimInPlace(newValue);

    if (strcmp(newValue, state.hostname()) != 0) {
        state.hostname(newValue);
    }

    snprintf((char*)reply.data, sizeof(reply.data), "%s", state.hostname());
    return reply;
}

void Api::formatReply(const Reply& reply, char* buf, size_t bufSize) {
    snprintf(buf, bufSize, "API [%s%s%s] (%s) %s",
             reply.command,
             reply.args[0] != '\0' ? " " : "",
             reply.args,
             Reply::codeToString(reply.code),
             (char*)reply.data);
}

#ifdef FEATURE_SERIAL
void Api::handleSerialInput() {
    static char serialBuf[128] = {};
    static size_t serialBufLen = 0;

    while (Serial.available()) {
        char c = Serial.read();
        // Echo printable characters only.  We MUST NOT echo \r because the
        // command is now handled *synchronously* inside this same call —
        // echoing \r would move the cursor back to column 0, and then
        // formatReplyToSerial() would overwrite the user's input line.
        if (c >= ' ') {
            Serial.print(c);
        }
        if (c == '\n' || c == '\r') {
            // Move to a fresh line before printing the reply (the \r echo
            // was already suppressed above).
            Serial.println();
            if (serialBufLen > 0) {
                serialBuf[serialBufLen] = '\0';
                Util::trimInPlace(serialBuf);
                if (serialBuf[0] != '\0') {
                    Reply reply = handleCommand(serialBuf);
                    formatReplyToSerial(reply);
                }
                serialBufLen = 0;
                serialBuf[0] = '\0';
            }
        } else {
            if (serialBufLen < sizeof(serialBuf) - 1) {
                serialBuf[serialBufLen++] = c;
            } else {
                ESP_LOGE(taskName(), "Serial buffer overflow");
                serialBufLen = 0;
                serialBuf[0] = '\0';
            }
        }
    }
}

void Api::formatReplyToSerial(const Reply& reply) {
    char line[REPLY_SERIAL_LINE_SIZE] = {};
    formatReply(reply, line, sizeof(line));
    Serial.print(line);
}
#endif

const char* Api::Reply::codeToString(Code code) {
    switch (code) {
        case Code::Success:
            return "Success";
        case Code::UnknownCommand:
            return "Unknown Command";
        case Code::InvalidArgs:
            return "Invalid Arguments";
        case Code::ExecutionError:
            return "Execution Error";
    }
    return "Unknown";
}
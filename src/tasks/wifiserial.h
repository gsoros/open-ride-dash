#ifndef WIFISERIAL_H
#define WIFISERIAL_H

#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstring>
#include "task.h"
#include "api.h"
#include "config.h"

class WifiSerial : public Task, public ApiClient {
   public:
    static constexpr size_t LINE_SIZE = 300;
    static constexpr size_t NUM_LOG_LINES = 8;

    WifiSerial(uint16_t port = 23);

    virtual const char* taskName() const override;
    virtual void setup(bool withWifi = true);
    virtual void taskRun() override;

    void receiveReply(const Api::Reply& reply) override;

    void setEcho(bool enable);

    static int queue_vprintf(const char* fmt, va_list args);

    void drainLogQueue();

    void flush();

    void disconnectWithNotice(const char* message);

   protected:
    Api::Reply setEchoCommand(const char* args);

    // std=GNU++17 allows inline static member variables
    inline static WifiSerial* instance = nullptr;
    uint16_t port = 23;
    WiFiServer wifiServer{port};
    WiFiClient wifiClient;
    bool echo = false;
    bool logClientActive = false;

    struct LogLine {
        char text[LINE_SIZE];
    };
    QueueHandle_t logQueue = nullptr;
};

extern WifiSerial wifiSerial;

#endif  // WIFISERIAL_H
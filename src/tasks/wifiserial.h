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
    static bool parseEchoValue(const char* args, bool* value);
    Api::Reply setEchoCommand(const char* args);

    // std=GNU++17 allows inline static member variables
    inline static WifiSerial* instance = nullptr;
    uint16_t port = 23;
    WiFiServer wifiServer{port};
    WiFiClient wifiClient;
    bool echo = false;
    bool logClientActive = false;

    struct LogLine {
        // 200 chars covers the usual app log lines with room to
        // spare; bump if something legitimately gets truncated.
        char text[200];
    };
    QueueHandle_t logQueue = nullptr;
};

extern WifiSerial wifiSerial;

#endif  // WIFISERIAL_H
#include <Arduino.h>

#define LED 8

void setup() {
    pinMode(LED, OUTPUT);
    Serial.begin(115200);
}

void loop() {
    digitalWrite(LED, LOW);
    delay(50);
    digitalWrite(LED, HIGH);
    delay(950);
    Serial.printf("Hello world! millis=%lu\n", millis());
}
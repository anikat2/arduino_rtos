#include <Arduino.h>
#include <rtos/RTOS.h>

RTOS rtos;

/* Runs ahead of telemetry. delay() leaves gaps so the lower task can run. */
void motor_control(void*) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        rtos.delay(100);
    }
}

void telemetry(void*) {
    pinMode(12, OUTPUT);
    for (;;) {
        digitalWrite(12, !digitalRead(12));
        rtos.delay(500);
    }
}

void setup() {
    rtos.createTask(motor_control, nullptr, 2);
    rtos.createTask(telemetry, nullptr, 1);
    rtos.start();
}

void loop() {
}

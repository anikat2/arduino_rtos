#pragma once

#include <stdint.h>
#include "rtos/Task.h"

/*
 * 8 tasks * 128 byte stacks is 1KB, half of an Uno's SRAM.
 * shrink MAX_TASKS or STACK_SIZE if the sketch runs out of memory.
 * about 40 of those bytes hold the saved registers.
 */
#define MAX_TASKS 8
#define STACK_SIZE 128
#define IDLE_STACK_SIZE 96

extern "C" uint16_t rtos_switch(uint16_t saved_sp, uint8_t from_tick);

class RTOS {
private:
    Task tasks[MAX_TASKS];
    uint8_t stacks[MAX_TASKS][STACK_SIZE];
    Task idle;
    uint8_t idle_stack[IDLE_STACK_SIZE];

    uint8_t task_count {0};
    int8_t current_index {-1};
    Task* current {nullptr};
    Task* faulted_task {nullptr};
    Task* volatile sleep_task {nullptr};
    volatile uint16_t sleep_ms {0};
    volatile uint16_t tick_count {0};
    volatile bool started {false};

    friend uint16_t rtos_switch(uint16_t saved_sp, uint8_t from_tick);

    static void taskShell(void* raw);
    static uint16_t shellWord();
    void initIdle();
    void scheduleNext();
    uint16_t onSwitch(uint16_t saved_sp, uint8_t from_tick);

public:
    RTOS();
    RTOS(const RTOS&) = delete;
    RTOS& operator=(const RTOS&) = delete;

    Task* createTask(
        void (*func)(void*),
        void* arg,
        uint8_t priority
    );

    bool start();

    void yield();
    void delay(uint16_t ms);
    void suspend(Task* task);
    void resume(Task* task);

    uint16_t ticks();
    Task* faultedTask();
};

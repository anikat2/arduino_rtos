#pragma once

#include <stdint.h>
#include "rtos/Task.h"

struct SchedView {
    uint8_t priority;
    TaskState state;
};

struct MlfqLevel {
    uint8_t priority;
    uint8_t base_priority;
    uint8_t quantum;
};

int8_t select_ready(const SchedView* tasks, uint8_t count, int8_t current);

uint8_t mlfq_slice(uint8_t base_priority, uint8_t priority);
void mlfq_account(MlfqLevel* level, uint8_t used_tick);
void mlfq_boost(MlfqLevel* level);

uint8_t* build_initial_stack(uint8_t* top, uint16_t entry_word, uint16_t arg_word);

#include "rtos/schedule.h"

#if defined(__AVR__)
#include <avr/io.h>
#endif

int8_t select_ready(const SchedView* tasks, uint8_t count, int8_t current) {
    int8_t best = -1;
    uint8_t best_priority = 0;

    for (uint8_t n = 0; n < count; n++) {
        uint8_t index;
        if (current < 0) {
            index = n;
        } else {
            index = (uint8_t)((uint8_t)current + 1u + n);
            if (index >= count) {
                index = (uint8_t)(index - count);
            }
        }

        if (tasks[index].state != TaskState::Ready) {
            continue;
        }
        if (best < 0 || tasks[index].priority > best_priority) {
            best = (int8_t)index;
            best_priority = tasks[index].priority;
        }
    }

    return best;
}

uint8_t mlfq_slice(uint8_t base_priority, uint8_t priority) {
    uint8_t slice = MLFQ_TOP_SLICE;
    if (priority > base_priority) {
        return slice;
    }
    uint8_t drop = (uint8_t)(base_priority - priority);
    while (drop > 0 && slice < MLFQ_MAX_SLICE) {
        slice = (uint8_t)(slice * 2);
        drop--;
    }
    return slice;
}

void mlfq_account(MlfqLevel* level, uint8_t used_tick) {
    if (level->priority > level->base_priority) {
        level->priority = level->base_priority;
    }
    if (used_tick == 0) {
        level->quantum = mlfq_slice(level->base_priority, level->priority);
        return;
    }
    if (level->quantum > 0) {
        level->quantum--;
    }
    if (level->quantum == 0) {
        if (level->priority > 0) {
            level->priority--;
        }
        level->quantum = mlfq_slice(level->base_priority, level->priority);
    }
}

void mlfq_boost(MlfqLevel* level) {
    level->priority = level->base_priority;
    level->quantum = mlfq_slice(level->base_priority, level->priority);
}

static void push_byte(uint8_t*& sp, uint8_t value) {
    *sp = value;
    --sp;
}

uint8_t* build_initial_stack(uint8_t* top, uint16_t entry_word, uint16_t arg_word) {
    uint8_t* sp = top;

    push_byte(sp, (uint8_t)(entry_word & 0xFF));
    push_byte(sp, (uint8_t)(entry_word >> 8));
#if defined(__AVR_3_BYTE_PC__)
    push_byte(sp, 0);
#endif

    push_byte(sp, 0x00);
    push_byte(sp, 0x80);

#if defined(__AVR_HAVE_RAMPZ__)
    push_byte(sp, 0x00);
#endif
#if defined(__AVR_3_BYTE_PC__)
    push_byte(sp, 0x00);
#endif

    for (uint8_t reg = 1; reg <= 31; reg++) {
        if (reg == 24) {
            push_byte(sp, (uint8_t)(arg_word & 0xFF));
        } else if (reg == 25) {
            push_byte(sp, (uint8_t)(arg_word >> 8));
        } else {
            push_byte(sp, 0x00);
        }
    }

    return sp;
}

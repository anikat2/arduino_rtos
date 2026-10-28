#include "rtos/RTOS.h"

#ifndef __AVR__
#error "This RTOS context switch targets AVR Arduino boards."
#endif

#include "rtos/schedule.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <string.h>

#ifndef F_CPU
#error "F_CPU must be defined. The Arduino build sets it from the board clock."
#endif

static_assert(STACK_SIZE >= 48, "STACK_SIZE is smaller than a saved context");
static_assert(IDLE_STACK_SIZE >= 48, "idle stack is smaller than a saved context");
static_assert(sizeof(void (*)(void*)) == sizeof(uint16_t), "AVR function pointers are 16-bit word addresses");

static RTOS* g_rtos = nullptr;
static SchedView schedule_view[MAX_TASKS];

/*
 * Context frame, top of stack last:
 *   PC low, PC high, [PC high byte on 3-byte devices],
 *   r0, SREG, [RAMPZ], [EIND], r1..r31
 * RET pops the PC high byte first. RAMPZ then EIND matches build_initial_stack().
 */
#if defined(__AVR_HAVE_RAMPZ__) && defined(__AVR_HAVE_EIND__)
#define SAVE_EXTRA \
    "in r0, 0x3B\n\t" \
    "push r0\n\t" \
    "in r0, 0x3C\n\t" \
    "push r0\n\t"
#define RESTORE_EXTRA \
    "pop r0\n\t" \
    "out 0x3C, r0\n\t" \
    "pop r0\n\t" \
    "out 0x3B, r0\n\t"
#elif defined(__AVR_HAVE_RAMPZ__)
#define SAVE_EXTRA \
    "in r0, 0x3B\n\t" \
    "push r0\n\t"
#define RESTORE_EXTRA \
    "pop r0\n\t" \
    "out 0x3B, r0\n\t"
#elif defined(__AVR_HAVE_EIND__)
#define SAVE_EXTRA \
    "in r0, 0x3C\n\t" \
    "push r0\n\t"
#define RESTORE_EXTRA \
    "pop r0\n\t" \
    "out 0x3C, r0\n\t"
#else
#define SAVE_EXTRA
#define RESTORE_EXTRA
#endif

#define POP_CONTEXT \
    "pop r31\n\t" \
    "pop r30\n\t" \
    "pop r29\n\t" \
    "pop r28\n\t" \
    "pop r27\n\t" \
    "pop r26\n\t" \
    "pop r25\n\t" \
    "pop r24\n\t" \
    "pop r23\n\t" \
    "pop r22\n\t" \
    "pop r21\n\t" \
    "pop r20\n\t" \
    "pop r19\n\t" \
    "pop r18\n\t" \
    "pop r17\n\t" \
    "pop r16\n\t" \
    "pop r15\n\t" \
    "pop r14\n\t" \
    "pop r13\n\t" \
    "pop r12\n\t" \
    "pop r11\n\t" \
    "pop r10\n\t" \
    "pop r9\n\t" \
    "pop r8\n\t" \
    "pop r7\n\t" \
    "pop r6\n\t" \
    "pop r5\n\t" \
    "pop r4\n\t" \
    "pop r3\n\t" \
    "pop r2\n\t" \
    "pop r1\n\t" \
    RESTORE_EXTRA \
    "pop r0\n\t" \
    "out __SREG__, r0\n\t" \
    "pop r0\n\t" \
    "ret\n\t"

/* ori only encodes r16-r31, so the saved I-bit is set through r16. */
#define SWITCH_BODY(TICK_FLAG) \
    asm volatile( \
        "cli\n\t" \
        "push r0\n\t" \
        "in r0, __SREG__\n\t" \
        "push r16\n\t" \
        "mov r16, r0\n\t" \
        "ori r16, 0x80\n\t" \
        "mov r0, r16\n\t" \
        "pop r16\n\t" \
        "push r0\n\t" \
        SAVE_EXTRA \
        "push r1\n\t" \
        "clr r1\n\t" \
        "push r2\n\t" \
        "push r3\n\t" \
        "push r4\n\t" \
        "push r5\n\t" \
        "push r6\n\t" \
        "push r7\n\t" \
        "push r8\n\t" \
        "push r9\n\t" \
        "push r10\n\t" \
        "push r11\n\t" \
        "push r12\n\t" \
        "push r13\n\t" \
        "push r14\n\t" \
        "push r15\n\t" \
        "push r16\n\t" \
        "push r17\n\t" \
        "push r18\n\t" \
        "push r19\n\t" \
        "push r20\n\t" \
        "push r21\n\t" \
        "push r22\n\t" \
        "push r23\n\t" \
        "push r24\n\t" \
        "push r25\n\t" \
        "push r26\n\t" \
        "push r27\n\t" \
        "push r28\n\t" \
        "push r29\n\t" \
        "push r30\n\t" \
        "push r31\n\t" \
        "ldi r22, " TICK_FLAG "\n\t" \
        "ldi r23, 0\n\t" \
        "in r24, __SP_L__\n\t" \
        "in r25, __SP_H__\n\t" \
        "call rtos_switch\n\t" \
        "out __SP_L__, r24\n\t" \
        "out __SP_H__, r25\n\t" \
        POP_CONTEXT \
    )

extern "C" __attribute__((naked, used, noinline)) void rtos_switch_yield(void) {
    SWITCH_BODY("0");
}

extern "C" __attribute__((naked, used)) void rtos_switch_from_tick(void) {
    SWITCH_BODY("1");
}

extern "C" __attribute__((naked, used)) void rtos_start_first(uint16_t) {
    asm volatile(
        "out __SP_L__, r24\n\t"
        "out __SP_H__, r25\n\t"
        POP_CONTEXT
    );
}

extern "C" __attribute__((used)) uint16_t rtos_switch(uint16_t saved_sp, uint8_t from_tick) {
    if (g_rtos == nullptr) {
        return saved_sp;
    }
    return g_rtos->onSwitch(saved_sp, from_tick);
}

#if defined(__AVR_ATmega32U4__)
ISR(TIMER3_COMPA_vect, ISR_NAKED) {
    asm volatile("call rtos_switch_from_tick");
    asm volatile("reti");
}
#elif defined(TCCR2A) && defined(OCR2A)
ISR(TIMER2_COMPA_vect, ISR_NAKED) {
    asm volatile("call rtos_switch_from_tick");
    asm volatile("reti");
}
#else
#error "No tick timer for this AVR. Uno, Nano, Mega, and Leonardo are supported."
#endif

static void idle_body(void*) {
    for (;;) {
        sleep_mode();
    }
}

static void setup_tick_timer() {
#if defined(__AVR_ATmega32U4__)
    uint32_t counts = F_CPU / 64UL / 1000UL;
    if (counts < 1) {
        counts = 1;
    }
    if (counts > 65536UL) {
        counts = 65536UL;
    }

    TCCR3A = 0;
    TCCR3B = 0;
    TCNT3 = 0;
    OCR3A = (uint16_t)(counts - 1UL);
    TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
    TIFR3 = (1 << OCF3A);
    TIMSK3 |= (1 << OCIE3A);
#else
    const uint16_t prescales[] = {1, 8, 32, 64, 128, 256, 1024};
    const uint8_t bits[] = {
        (1 << CS20),
        (1 << CS21),
        (1 << CS21) | (1 << CS20),
        (1 << CS22),
        (1 << CS22) | (1 << CS20),
        (1 << CS22) | (1 << CS21),
        (1 << CS22) | (1 << CS21) | (1 << CS20)
    };

    uint8_t chosen_bits = bits[6];
    uint8_t ocr = 0;
    for (uint8_t i = 0; i < 7; i++) {
        uint32_t counts = F_CPU / prescales[i] / 1000UL;
        if (counts < 1 || counts > 256UL) {
            continue;
        }
        chosen_bits = bits[i];
        ocr = (uint8_t)(counts - 1UL);
        if ((F_CPU % (prescales[i] * 1000UL)) == 0) {
            break;
        }
    }

    TCCR2A = 0;
    TCCR2B = 0;
    TCNT2 = 0;
    OCR2A = ocr;
    TCCR2A = (1 << WGM21);
    TCCR2B = chosen_bits;
    TIFR2 = (1 << OCF2A);
    TIMSK2 |= (1 << OCIE2A);
#endif
}

RTOS::RTOS() {
    g_rtos = this;
}

uint16_t RTOS::shellWord() {
    void (*shell)(void*) = taskShell;
    uint16_t word = 0;
    memcpy(&word, &shell, sizeof(word));
    return word;
}

void RTOS::taskShell(void* raw) {
    Task* self = static_cast<Task*>(raw);
    void (*fn)(void*) = self->func;
    void* arg = self->arg;
    if (fn != nullptr) {
        fn(arg);
    }
    self->state = TaskState::Terminated;
    for (;;) {
        g_rtos->yield();
    }
}

void RTOS::initIdle() {
    uint8_t* base = idle_stack;
    uint8_t* sp = build_initial_stack(base + IDLE_STACK_SIZE - 1, shellWord(), (uint16_t)(uintptr_t)&idle);
    idle = Task(idle_body, nullptr, sp, base, IDLE_STACK_SIZE, 0);
}

void RTOS::scheduleNext() {
    for (uint8_t i = 0; i < task_count; i++) {
        schedule_view[i].priority = tasks[i].priority;
        schedule_view[i].state = tasks[i].state;
    }

    int8_t next = select_ready(schedule_view, task_count, current_index);
    if (next >= 0 && tasks[next].stack_pointer != nullptr) {
        current = &tasks[next];
        current_index = next;
    } else {
        current = &idle;
        current_index = -1;
    }
    current->state = TaskState::Running;
}

uint16_t RTOS::onSwitch(uint16_t saved_sp, uint8_t from_tick) {
    if (!started || current == nullptr) {
        return saved_sp;
    }

    uint8_t* sp = (uint8_t*)(uintptr_t)saved_sp;
    uint8_t* base = current->stack_base;
    if (base != nullptr && (sp < base || sp >= base + current->stack_capacity)) {
        current->overflowed_flag = 1;
        current->state = TaskState::Terminated;
        faulted_task = current;
    } else {
        current->stack_pointer = sp;
    }

    if (from_tick) {
        tick_count++;
        for (uint8_t i = 0; i < task_count; i++) {
            if (tasks[i].state != TaskState::Blocked) {
                continue;
            }
            if (tasks[i].delay_remaining > 0) {
                tasks[i].delay_remaining--;
            }
            if (tasks[i].delay_remaining == 0) {
                tasks[i].state = TaskState::Ready;
            }
        }
    }

    if (sleep_task != nullptr) {
        Task* sleeping = sleep_task;
        uint16_t ms = sleep_ms;
        sleep_task = nullptr;
        if (sleeping->state != TaskState::Terminated && ms > 0) {
            sleeping->delay_remaining = ms;
            sleeping->state = TaskState::Blocked;
        }
    }

    if (current->state == TaskState::Running) {
        current->state = TaskState::Ready;
    }

    scheduleNext();
    return (uint16_t)(uintptr_t)current->stack_pointer;
}

Task* RTOS::createTask(void (*func)(void*), void* arg, uint8_t priority) {
    if (func == nullptr || task_count >= MAX_TASKS) {
        return nullptr;
    }

    if (started) {
        cli();
    }

    uint8_t index = task_count;
    Task* slot = &tasks[index];
    uint8_t* base = stacks[index];
    uint8_t* sp = build_initial_stack(
        base + STACK_SIZE - 1,
        shellWord(),
        (uint16_t)(uintptr_t)slot
    );
    *slot = Task(func, arg, sp, base, STACK_SIZE, priority);
    task_count = (uint8_t)(index + 1);

    if (started) {
        sei();
    }
    return slot;
}

bool RTOS::start() {
    if (started || task_count == 0) {
        return false;
    }

    cli();
    initIdle();
    started = true;
    setup_tick_timer();
    set_sleep_mode(SLEEP_MODE_IDLE);
    current = nullptr;
    current_index = -1;
    scheduleNext();
    asm volatile("" ::: "memory");
    rtos_start_first((uint16_t)(uintptr_t)current->stack_pointer);
    return false;
}

__attribute__((noinline)) void RTOS::yield() {
    rtos_switch_yield();
}

void RTOS::delay(uint16_t ms) {
    if (!started) {
        return;
    }
    if (ms == 0) {
        yield();
        return;
    }

    cli();
    sleep_ms = ms;
    sleep_task = current;
    sei();
    yield();
}

void RTOS::suspend(Task* task) {
    if (task == nullptr) {
        return;
    }

    cli();
    bool self = (task == current);
    if (task->state == TaskState::Ready || task->state == TaskState::Running) {
        task->state = TaskState::Suspended;
    }
    sei();

    if (self && started) {
        yield();
    }
}

void RTOS::resume(Task* task) {
    if (task == nullptr) {
        return;
    }

    cli();
    if (task->state == TaskState::Suspended) {
        task->state = TaskState::Ready;
    }
    sei();

    if (started) {
        yield();
    }
}

uint16_t RTOS::ticks() {
    cli();
    uint16_t now = tick_count;
    sei();
    return now;
}

Task* RTOS::faultedTask() {
    cli();
    Task* task = faulted_task;
    sei();
    return task;
}

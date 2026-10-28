#pragma once

#include <stdint.h>

#define MLFQ_TOP_SLICE 2
#define MLFQ_MAX_SLICE 16
#define MLFQ_BOOST_TICKS 100

enum class TaskState : uint8_t {
    Ready,
    Running,
    Blocked,
    Suspended,
    Terminated
};

class RTOS;

class Task {
    friend class RTOS;

private:
    uint8_t priority;
    uint8_t base_priority;
    uint8_t quantum;
    TaskState state;
    uint16_t delay_remaining;
    uint8_t stack_capacity;
    uint8_t overflowed_flag;
    uint8_t* stack_pointer;
    uint8_t* stack_base;
    void (*func)(void*);
    void* arg;

public:
    Task()
    : priority{0},
      base_priority{0},
      quantum{MLFQ_TOP_SLICE},
      state{TaskState::Terminated},
      delay_remaining{0},
      stack_capacity{0},
      overflowed_flag{0},
      stack_pointer{nullptr},
      stack_base{nullptr},
      func{nullptr},
      arg{nullptr} {}

    Task(
        void (*func)(void*),
        void* arg,
        uint8_t* stack_pointer,
        uint8_t* stack_base,
        uint8_t stack_capacity,
        uint8_t priority
    ) : priority{priority},
        base_priority{priority},
        quantum{MLFQ_TOP_SLICE},
        state{TaskState::Ready},
        delay_remaining{0},
        stack_capacity{stack_capacity},
        overflowed_flag{0},
        stack_pointer{stack_pointer},
        stack_base{stack_base},
        func{func},
        arg{arg} {}

    uint8_t getPriority() const;
    uint8_t getBasePriority() const;
    void setPriority(uint8_t priority);

    TaskState getState() const;
    bool overflowed() const;

    uint8_t* getStackPointer();
    void setStackPointer(uint8_t* pter);
};

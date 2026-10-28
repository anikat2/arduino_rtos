#pragma once

#include <stdint.h>

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
        state{TaskState::Ready},
        delay_remaining{0},
        stack_capacity{stack_capacity},
        overflowed_flag{0},
        stack_pointer{stack_pointer},
        stack_base{stack_base},
        func{func},
        arg{arg} {}

    uint8_t getPriority() const;
    void setPriority(uint8_t priority);

    TaskState getState() const;
    bool overflowed() const;

    uint8_t* getStackPointer();
    void setStackPointer(uint8_t* pter);
};

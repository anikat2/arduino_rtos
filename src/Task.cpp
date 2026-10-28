#include "rtos/Task.h"

uint8_t Task::getPriority() const {
    return priority;
}

void Task::setPriority(uint8_t priority) {
    this->priority = priority;
}

TaskState Task::getState() const {
    return state;
}

bool Task::overflowed() const {
    return overflowed_flag != 0;
}

uint8_t* Task::getStackPointer() {
    return stack_pointer;
}

void Task::setStackPointer(uint8_t* pter) {
    stack_pointer = pter;
}

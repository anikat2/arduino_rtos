#include "rtos/Task.h"
#include "rtos/schedule.h"

uint8_t Task::getPriority() const {
    return priority;
}

uint8_t Task::getBasePriority() const {
    return base_priority;
}

void Task::setPriority(uint8_t priority) {
    this->base_priority = priority;
    this->priority = priority;
    this->quantum = mlfq_slice(priority, priority);
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

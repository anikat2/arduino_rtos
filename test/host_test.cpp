#include "rtos/schedule.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define EXPECT(cond) do { \
    if (!(cond)) { \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        failures++; \
    } \
} while (0)

static void test_priority_and_rotation() {
    SchedView tasks[3];

    tasks[0] = {1, TaskState::Ready};
    tasks[1] = {1, TaskState::Ready};
    tasks[2] = {1, TaskState::Ready};
    EXPECT(select_ready(tasks, 3, 0) == 1);
    EXPECT(select_ready(tasks, 3, 1) == 2);
    EXPECT(select_ready(tasks, 3, 2) == 0);
    EXPECT(select_ready(tasks, 3, -1) == 0);

    tasks[0] = {1, TaskState::Ready};
    tasks[1] = {5, TaskState::Ready};
    tasks[2] = {5, TaskState::Ready};
    EXPECT(select_ready(tasks, 3, 0) == 1);
    EXPECT(select_ready(tasks, 3, 1) == 2);
    EXPECT(select_ready(tasks, 3, 2) == 1);
    EXPECT(select_ready(tasks, 3, -1) == 1);

    tasks[1].state = TaskState::Blocked;
    EXPECT(select_ready(tasks, 3, 0) == 2);
    EXPECT(select_ready(tasks, 3, 2) == 2);

    tasks[0].state = TaskState::Suspended;
    tasks[2].state = TaskState::Terminated;
    EXPECT(select_ready(tasks, 3, -1) == -1);

    SchedView only = {3, TaskState::Ready};
    EXPECT(select_ready(&only, 1, 0) == 0);
    only.state = TaskState::Blocked;
    EXPECT(select_ready(&only, 1, 0) == -1);
    EXPECT(select_ready(tasks, 0, -1) == -1);
}

static void test_initial_stack() {
    uint8_t mem[80];
    memset(mem, 0xAB, sizeof(mem));
    uint8_t* top = mem + 70;
    mem[71] = 0xCD;

    uint8_t* sp = build_initial_stack(top, 0x1234, 0xABCD);

    EXPECT(sp == top - 35);
    EXPECT(mem[71] == 0xCD);
    EXPECT(sp[0] == 0xAB);
    EXPECT(top[0] == 0x34);
    EXPECT(top[-1] == 0x12);
    EXPECT(top[-2] == 0x00);
    EXPECT(top[-3] == 0x80);
    EXPECT(top[-4] == 0x00);
    EXPECT(top[-27] == 0xCD);
    EXPECT(top[-28] == 0xAB);
    EXPECT(top[-26] == 0x00);
    EXPECT(top[-29] == 0x00);
    EXPECT(top[-34] == 0x00);

    for (int reg = 1; reg <= 31; reg++) {
        if (reg == 24 || reg == 25) {
            continue;
        }
        EXPECT(top[-(3 + reg)] == 0x00);
    }
}

int main() {
    test_priority_and_rotation();
    test_initial_stack();
    if (failures != 0) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("ok\n");
    return 0;
}

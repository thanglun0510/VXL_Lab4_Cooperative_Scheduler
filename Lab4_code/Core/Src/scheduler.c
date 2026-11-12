#include "scheduler.h"
#include "main.h"

static sTask SCH_tasks_G[SCH_MAX_TASKS];
static uint32_t current_task_id = 0;
static volatile uint32_t missed_ticks = 0;


static void remove_at(uint8_t index) {
    if (index + 1 < SCH_MAX_TASKS && SCH_tasks_G[index + 1].pTask != 0) {
        SCH_tasks_G[index + 1].Delay += SCH_tasks_G[index].Delay;
    }
    for (int i = index; i < SCH_MAX_TASKS - 1; i++) {
        SCH_tasks_G[i] = SCH_tasks_G[i + 1];
    }
    SCH_tasks_G[SCH_MAX_TASKS - 1].pTask  = 0;
    SCH_tasks_G[SCH_MAX_TASKS - 1].Delay  = 0;
    SCH_tasks_G[SCH_MAX_TASKS - 1].Period = 0;
    SCH_tasks_G[SCH_MAX_TASKS - 1].RunMe  = 0;
    SCH_tasks_G[SCH_MAX_TASKS - 1].TaskID = SCH_ERROR_ID;
}

static uint32_t insert_task(void (*pFunction)(), uint32_t DELAY, uint32_t PERIOD, uint32_t id) {
    if (SCH_tasks_G[SCH_MAX_TASKS - 1].pTask != 0) return SCH_ERROR_ID;

    uint8_t index = 0;
    while ((index < SCH_MAX_TASKS) && (SCH_tasks_G[index].pTask != 0)
           && (SCH_tasks_G[index].Delay <= DELAY)) {
        DELAY -= SCH_tasks_G[index].Delay;
        index++;
    }

    for (int i = SCH_MAX_TASKS - 1; i > index; i--) {
        SCH_tasks_G[i] = SCH_tasks_G[i - 1];
    }

    SCH_tasks_G[index].pTask  = pFunction;
    SCH_tasks_G[index].Delay  = DELAY;
    SCH_tasks_G[index].Period = PERIOD;
    SCH_tasks_G[index].RunMe  = 0;
    SCH_tasks_G[index].TaskID = id;

    if (index + 1 < SCH_MAX_TASKS && SCH_tasks_G[index + 1].pTask != 0) {
        SCH_tasks_G[index + 1].Delay -= DELAY;
    }
    return id;
}


void SCH_Init(void) {
    for (int i = 0; i < SCH_MAX_TASKS; i++) {
        SCH_tasks_G[i].pTask  = 0;
        SCH_tasks_G[i].Delay  = 0;
        SCH_tasks_G[i].Period = 0;
        SCH_tasks_G[i].RunMe  = 0;
        SCH_tasks_G[i].TaskID = SCH_ERROR_ID;
    }
    current_task_id = 0;
}

void SCH_Update(void) {
    if (SCH_tasks_G[0].pTask) {
        if (SCH_tasks_G[0].Delay > 0) {
            SCH_tasks_G[0].Delay--;
            if (SCH_tasks_G[0].Delay == 0) {
                SCH_tasks_G[0].RunMe = 1;
            }
        } else {
            SCH_tasks_G[0].RunMe = 1;
            missed_ticks++;
        }
    }
}

uint32_t SCH_Add_Task(void (*pFunction)(), uint32_t DELAY, uint32_t PERIOD) {
    uint32_t pm = __get_PRIMASK();
    __disable_irq();

    uint32_t id = SCH_ERROR_ID;
    if (SCH_tasks_G[SCH_MAX_TASKS - 1].pTask == 0) {
        id = insert_task(pFunction, DELAY, PERIOD, current_task_id);
        if (id != SCH_ERROR_ID) current_task_id++;
    }

    __set_PRIMASK(pm);
    return id;
}

void SCH_Dispatch_Tasks(void) {
    while (1) {
        uint32_t pm = __get_PRIMASK();
        __disable_irq();

        if (!SCH_tasks_G[0].pTask) {
            missed_ticks = 0;
        }
        while (missed_ticks > 0 && SCH_tasks_G[0].pTask && SCH_tasks_G[0].Delay > 0) {
            SCH_tasks_G[0].Delay--;
            missed_ticks--;
            if (SCH_tasks_G[0].Delay == 0) {
                SCH_tasks_G[0].RunMe = 1;
            }
        }

        if (!(SCH_tasks_G[0].pTask && SCH_tasks_G[0].RunMe > 0)) {
            __set_PRIMASK(pm);
            break;
        }

        sTask t = SCH_tasks_G[0];
        remove_at(0);

        if (t.Period > 0) {
            insert_task(t.pTask, t.Period, t.Period, t.TaskID);
        }

        if (SCH_tasks_G[0].pTask && SCH_tasks_G[0].Delay == 0) {
            SCH_tasks_G[0].RunMe = 1;
        }

        __set_PRIMASK(pm);

        (*t.pTask)();
    }
}

uint8_t SCH_Delete_Task(uint32_t taskID) {
    uint32_t pm = __get_PRIMASK();
    __disable_irq();

    uint8_t found = 0;
    for (uint8_t index = 0; index < SCH_MAX_TASKS; index++) {
        if (SCH_tasks_G[index].pTask != 0 && SCH_tasks_G[index].TaskID == taskID) {
            remove_at(index);
            found = 1;
            break;
        }
    }

    __set_PRIMASK(pm);
    return found;
}

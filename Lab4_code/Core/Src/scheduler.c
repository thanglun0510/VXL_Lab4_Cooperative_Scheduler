#include "scheduler.h"

sTask SCH_tasks_G[SCH_MAX_TASKS];
static uint32_t current_task_id = 0; // Biến đếm để cấp ID duy nhất cho mỗi task

void SCH_Init(void) {
    for (int i = 0; i < SCH_MAX_TASKS; i++) {
        SCH_tasks_G[i].pTask = 0;
        SCH_tasks_G[i].Delay = 0;
        SCH_tasks_G[i].Period = 0;
        SCH_tasks_G[i].RunMe = 0;
        SCH_tasks_G[i].TaskID = 0xFFFFFFFF;
    }
}

void SCH_Update(void) {
    // Thuật toán O(1): Chỉ cần kiểm tra task đứng đầu hàng đợi
    if (SCH_tasks_G[0].pTask) {
        if (SCH_tasks_G[0].Delay == 0) {
            SCH_tasks_G[0].RunMe += 1;
        } else {
            SCH_tasks_G[0].Delay -= 1;
            if (SCH_tasks_G[0].Delay == 0) {
                SCH_tasks_G[0].RunMe += 1;
            }
        }
    }
}

uint32_t SCH_Add_Task(void (*pFunction)(), uint32_t DELAY, uint32_t PERIOD) {
    uint8_t index = 0;

    // Tìm vị trí chèn task sao cho mảng luôn được sắp xếp theo thời gian Delay tăng dần
    while ((index < SCH_MAX_TASKS) && (SCH_tasks_G[index].pTask != 0) && (SCH_tasks_G[index].Delay <= DELAY)) {
        DELAY -= SCH_tasks_G[index].Delay; // Trừ lùi thời gian của các task xếp trước
        index++;
    }

    if (index == SCH_MAX_TASKS) return SCH_MAX_TASKS;

    // Dịch các task phía sau xuống 1 ô để nhường chỗ
    for (int i = SCH_MAX_TASKS - 1; i > index; i--) {
        SCH_tasks_G[i] = SCH_tasks_G[i - 1];
    }

    SCH_tasks_G[index].pTask = pFunction;
    SCH_tasks_G[index].Delay = DELAY;
    SCH_tasks_G[index].Period = PERIOD;
    SCH_tasks_G[index].RunMe = 0;
    SCH_tasks_G[index].TaskID = current_task_id++;

    // Trừ thời gian Delay của task liền kề phía sau để không làm sai lệch bộ đếm
    if (index + 1 < SCH_MAX_TASKS && SCH_tasks_G[index + 1].pTask != 0) {
        SCH_tasks_G[index + 1].Delay -= DELAY;
    }

    return SCH_tasks_G[index].TaskID;
}

void SCH_Dispatch_Tasks(void) {
    if (SCH_tasks_G[0].RunMe > 0) {
        (*SCH_tasks_G[0].pTask)(); // Chạy task đầu tiên
        SCH_tasks_G[0].RunMe -= 1;

        sTask temp = SCH_tasks_G[0];

        // Xóa task đầu tiên khỏi hàng đợi (các task sau sẽ tự động dịch lên)
        SCH_Delete_Task(temp.TaskID);

        // Nếu là task lặp lại (chu kỳ), thêm nó trở lại hàng đợi
        if (temp.Period > 0) {
            SCH_Add_Task(temp.pTask, temp.Period, temp.Period);
        }
    }
}

uint8_t SCH_Delete_Task(uint32_t taskID) {
    uint8_t index = 0;
    uint8_t found = 0;

    for (index = 0; index < SCH_MAX_TASKS; index++) {
        if (SCH_tasks_G[index].TaskID == taskID && SCH_tasks_G[index].pTask != 0) {
            found = 1;
            break;
        }
    }

    if (!found) return 0;

    // Cộng dồn thời gian Delay của task bị xóa vào task liền kề phía sau
    if (index + 1 < SCH_MAX_TASKS && SCH_tasks_G[index + 1].pTask != 0) {
        SCH_tasks_G[index + 1].Delay += SCH_tasks_G[index].Delay;
    }

    // Dịch các task phía sau lên 1 ô để lấp khoảng trống
    for (int i = index; i < SCH_MAX_TASKS - 1; i++) {
        SCH_tasks_G[i] = SCH_tasks_G[i + 1];
    }

    SCH_tasks_G[SCH_MAX_TASKS - 1].pTask = 0;
    SCH_tasks_G[SCH_MAX_TASKS - 1].Delay = 0;
    SCH_tasks_G[SCH_MAX_TASKS - 1].Period = 0;
    SCH_tasks_G[SCH_MAX_TASKS - 1].TaskID = 0xFFFFFFFF;

    return 1;
}

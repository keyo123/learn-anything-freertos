#include "demos.h"
#include "event_groups.h"
#include <stdio.h>

#define BIT_SENSOR  (1 << 0)
#define BIT_CONFIRM (1 << 1)

static EventGroupHandle_t xEventGroup = NULL;

static void vTaskSensor(void *pvParams)
{
    (void)pvParams;
    int i;

    for (i = 1; i <= 5; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(200));
        printfSafe("[Sensor]  Sample #%d\r\n", i);
        xEventGroupSetBits(xEventGroup, BIT_SENSOR);
    }

    vTaskDelete(NULL);
}

static void vTaskUser(void *pvParams)
{
    (void)pvParams;
    int i;

    for (i = 1; i <= 5; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(300));
        printfSafe("[User]    Confirm #%d\r\n", i);
        xEventGroupSetBits(xEventGroup, BIT_CONFIRM);
    }

    vTaskDelete(NULL);
}

static void vTaskProcessor(void *pvParams)
{
    (void)pvParams;
    int count = 0;

    while (count < 5)
    {
        EventBits_t bits = xEventGroupWaitBits(
            xEventGroup,
            BIT_SENSOR | BIT_CONFIRM,
            pdTRUE,       /* 唤醒后自动清除 */
            pdTRUE,       /* AND 模式 */
            portMAX_DELAY
        );

        count++;
        printfSafe(">>> Processor: Process #%d (bits=0x%02x)\r\n", count, bits);
    }

    printfSafe("\r\n=== Event Group Demo Done ===\r\n");
    vTaskDelete(NULL);
}

/* ========================================================================
 * 2. 同步屏障 (Sync Barrier) 演示 (xEventGroupSync)
 * ======================================================================== */
#define BIT_TASK_A  (1 << 2)
#define BIT_TASK_B  (1 << 3)
#define BIT_TASK_C  (1 << 4)
#define ALL_SYNC_BITS (BIT_TASK_A | BIT_TASK_B | BIT_TASK_C)

static void vTaskSyncWorker(void *pvParams)
{
    int task_id = (int)pvParams;
    EventBits_t uxBitToSet = 0;

    if (task_id == 1) uxBitToSet = BIT_TASK_A;
    else if (task_id == 2) uxBitToSet = BIT_TASK_B;
    else if (task_id == 3) uxBitToSet = BIT_TASK_C;

    /* 模拟各任务不同长度的初始化/第一阶段工作时间 */
    vTaskDelay(pdMS_TO_TICKS(task_id * 1000));
    printfSafe("[Worker %d] Stage 1 finished. Waiting at Sync Barrier...\r\n", task_id);

    /* 
     * 调用 xEventGroupSync：
     * 1. 将自己的标志位置 1 (uxBitToSet)
     * 2. 阻塞等待所有相关的标志位 (ALL_SYNC_BITS) 都被置位
     * 3. 满足条件后自动清除相关标志位，并同时唤醒所有任务
     */
    xEventGroupSync(
        xEventGroup,
        uxBitToSet,
        ALL_SYNC_BITS,
        portMAX_DELAY
    );

    /* 所有任务跨越屏障后，同时进入第二阶段 */
    printfSafe("[Worker %d] Passed barrier! Entering Stage 2...\r\n", task_id);
    
    vTaskDelete(NULL);
}

void start_event_demos(void)
{
    xEventGroup = xEventGroupCreate();
    if (xEventGroup != NULL)
    {
        /* --- 演示 1：AND/OR 多事件等待 (取消注释可测试) --- */
        // xTaskCreate(vTaskSensor,    "Sensor",    configMINIMAL_STACK_SIZE, NULL, 1, NULL);
        // xTaskCreate(vTaskUser,      "User",      configMINIMAL_STACK_SIZE, NULL, 1, NULL);
        // xTaskCreate(vTaskProcessor, "Processor", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

        /* --- 演示 2：同步屏障 (Sync Barrier) 演示 (已启用) --- */
        printfSafe("\r\n=== Start Event Group Sync Barrier Demo ===\r\n");
        xTaskCreate(vTaskSyncWorker, "Worker1", configMINIMAL_STACK_SIZE, (void *)1, 1, NULL);
        xTaskCreate(vTaskSyncWorker, "Worker2", configMINIMAL_STACK_SIZE, (void *)2, 1, NULL);
        xTaskCreate(vTaskSyncWorker, "Worker3", configMINIMAL_STACK_SIZE, (void *)3, 1, NULL);
    }
}

/**
 * FreeRTOS 任务创建与删除练习
 * 
 * 补全 TODO 部分。
 */

#include "FreeRTOS.h"
#include "task.h"
#include "bsp_led.h"
#include "demos.h"

/* 任务句柄声明 */
static TaskHandle_t xSupervisorTaskHandle = NULL;
static TaskHandle_t xWorkerTaskHandle = NULL;

/* 声明任务函数 */
static void vTaskSupervisor(void *pvParameters);
static void vTaskWorker(void *pvParameters);

/**
 * @brief  工作任务：执行一段循环，打印剩余堆栈，并自我销毁
 */
static void vTaskWorker(void *pvParameters)
{
    int i;
    UBaseType_t uxHighWaterMark;

    printfSafe("[Worker] Dynamic task started!\r\n");

    for (i = 0; i < 5; i++)
    {
        LED2_TOGGLE;
        printfSafe("[Worker] Working step %d...\r\n", i + 1);

        /* TODO: 获取并打印当前任务的历史最小剩余堆栈大小（高水位线） */
        // uxHighWaterMark = uxTaskGetStackHighWaterMark( ... );
        // printfSafe("[Worker] Stack High Water Mark: %d words\r\n", (int)uxHighWaterMark);

        vTaskDelay(pdMS_TO_TICKS(500)); // 延时 500ms
    }

    printfSafe("[Worker] Work completed! Deleting self...\r\n");

    /* TODO: 调用 API 自我删除 */
    // vTaskDelete( ... );
}

/**
 * @brief  监视/主管任务：周期性动态创建工作任务
 */
static void vTaskSupervisor(void *pvParameters)
{
    (void)pvParameters;
    BaseType_t xReturn;
    int cycle_count = 0;

    for (;;)
    {
        cycle_count++;
        printfSafe("\r\n[Supervisor] Cycle %d: Creating worker task...\r\n", cycle_count);

        /* TODO: 动态创建 vTaskWorker 任务
         * - 任务名: "Worker"
         * - 堆栈深度: 128
         * - 任务参数: NULL
         * - 优先级: 1 (比 Supervisor 优先级 2 低)
         * - 传出句柄: &xWorkerTaskHandle
         */
        // xReturn = xTaskCreate( ... );

        if (xReturn == pdPASS)
        {
            printfSafe("[Supervisor] Worker task created successfully!\r\n");
        }
        else
        {
            printfSafe("[Supervisor] Failed to create worker task!\r\n");
        }

        /* 延时 6 秒后进行下一次循环（留给工作任务足够的时间运行并完成自我销毁） */
        vTaskDelay(pdMS_TO_TICKS(6000));
    }
}

/**
 * @brief  启动任务创建与删除演示的入口函数
 */
void start_task_demo(void)
{
    /* 创建主管任务，优先级设为 2 */
    xTaskCreate((TaskFunction_t)vTaskSupervisor,
                (const char *)"Supervisor",
                (uint16_t)256,
                (void *)NULL,
                (UBaseType_t)2,
                (TaskHandle_t *)&xSupervisorTaskHandle);
}

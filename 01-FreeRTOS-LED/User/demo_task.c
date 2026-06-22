/**
 * FreeRTOS 任务创建与删除练习
 *
 * 补全 TODO 部分。
 */

#include "FreeRTOS.h"
#include "bsp_led.h"
#include "demos.h"
#include "task.h"

/* 任务句柄声明 */
static TaskHandle_t xSupervisorTaskHandle = NULL;
static TaskHandle_t xWorkerTaskHandle = NULL;

/* 声明任务函数 */
static void vTaskSupervisor(void *pvParameters);
static void vTaskWorker(void *pvParameters);

/**
 * @brief  工作任务：执行一段循环，打印剩余堆栈，并自我销毁
 */
static void vTaskWorker(void *pvParameters) {
  (void)pvParameters;
  int i;
  UBaseType_t uxHighWaterMark;

  printfSafe("[Worker] Dynamic task started!\r\n");

  for (i = 0; i < 5; i++) {
    LED2_TOGGLE;
    printfSafe("[Worker] Working step %d...\r\n", i + 1);

    /* TODO: 获取并打印当前任务的历史最小剩余堆栈大小（高水位线） */
    uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    printfSafe("[Worker] Stack High Water Mark: %d words\r\n",
               (int)uxHighWaterMark);

    vTaskDelay(pdMS_TO_TICKS(500)); // 延时 500ms
  }

  printfSafe("[Worker] Work completed! Deleting self...\r\n");

  /* TODO: 调用 API 自我删除 */
  vTaskDelete(NULL);
}

/**
 * @brief  监视/主管任务：周期性动态创建工作任务
 */
static void vTaskSupervisor(void *pvParameters) {
  (void)pvParameters;
  BaseType_t xReturn;
  int cycle_count = 0;

  for (;;) {
    cycle_count++;
    printfSafe("\r\n[Supervisor] Cycle %d: Creating worker task...\r\n",
               cycle_count);

    /* TODO: 动态创建 vTaskWorker 任务
     * - 任务名: "Worker"
     * - 堆栈深度: 128
     * - 任务参数: NULL
     * - 优先级: 1 (比 Supervisor 优先级 2 低)
     * - 传出句柄: &xWorkerTaskHandle
     */
    xReturn =
        xTaskCreate(vTaskWorker, "Worker", 128, NULL, 1, &xWorkerTaskHandle);

    if (xReturn == pdPASS) {
      printfSafe("[Supervisor] Worker task created successfully!\r\n");
    } else {
      printfSafe("[Supervisor] Failed to create worker task!\r\n");
    }

    /* 延时 6 秒后进行下一次循环（留给工作任务足够的时间运行并完成自我销毁） */
    vTaskDelay(pdMS_TO_TICKS(6000));
  }
}

/**
 * @brief  启动任务创建与删除演示的入口函数
 */
void start_task_demo(void) {
  /* 创建主管任务，优先级设为 2 */
  xTaskCreate((TaskFunction_t)vTaskSupervisor, (const char *)"Supervisor",
              (uint16_t)256, (void *)NULL, (UBaseType_t)2,
              (TaskHandle_t *)&xSupervisorTaskHandle);
}

/* ========================================================================
 * 5. 相对延时 vTaskDelay vs 绝对延时 vTaskDelayUntil 对比演示
 * ======================================================================== */

static void vTaskRelativeDelay(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xStart = xTaskGetTickCount();
    /* 模拟波动的任务耗时 (工作时间) */
    const TickType_t xWorkTimes[5] = {
        pdMS_TO_TICKS(100),
        pdMS_TO_TICKS(300),
        pdMS_TO_TICKS(200),
        pdMS_TO_TICKS(400),
        pdMS_TO_TICKS(100)
    };
    int i;

    printfSafe("[Relative] Task started. Target: 1000ms sleep, fluctuating work.\r\n");

    for (i = 0; i < 5; i++)
    {
        TickType_t xWakeTime = xTaskGetTickCount() - xStart;
        printfSafe("[Relative] Loop %d Wakeup: %u ms (Work was %u ms)\r\n", 
                   i + 1, 
                   (unsigned int)(xWakeTime * portTICK_PERIOD_MS),
                   (unsigned int)(xWorkTimes[i] * portTICK_PERIOD_MS));

        /* 模拟波动的任务数据处理耗时 */
        vTaskDelay(xWorkTimes[i]);

        /* 相对延时 1000ms */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    printfSafe("[Relative] Task finished!\r\n");
    vTaskDelete(NULL);
}

static void vTaskAbsoluteDelay(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xStart = xTaskGetTickCount();
    TickType_t xLastWakeTime = xStart;
    const TickType_t xPeriod = pdMS_TO_TICKS(1500); /* 设为 1500ms 周期，确保大于最大的 work time + scheduling margin */
    /* 模拟相同的波动任务耗时 */
    const TickType_t xWorkTimes[5] = {
        pdMS_TO_TICKS(100),
        pdMS_TO_TICKS(300),
        pdMS_TO_TICKS(200),
        pdMS_TO_TICKS(400),
        pdMS_TO_TICKS(100)
    };
    int i;

    printfSafe("[Absolute] Task started. Target Period: 1500ms constant, fluctuating work.\r\n");

    for (i = 0; i < 5; i++)
    {
        /* 绝对延时：保证严格以 1500ms 为周期唤醒 */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        TickType_t xWakeTime = xLastWakeTime - xStart;
        printfSafe("[Absolute] Loop %d Wakeup: %u ms (Work was %u ms)\r\n", 
                   i + 1, 
                   (unsigned int)(xWakeTime * portTICK_PERIOD_MS),
                   (unsigned int)(xWorkTimes[i] * portTICK_PERIOD_MS));

        /* 模拟波动的任务数据处理耗时 */
        vTaskDelay(xWorkTimes[i]);
    }

    printfSafe("[Absolute] Task finished!\r\n");
    vTaskDelete(NULL);
}

void start_delay_demo(void)
{
    printfSafe("\r\n=== Start Delay Comparison Demo ===\r\n");
    
    /* 创建相对延时任务，优先级为 1 */
    xTaskCreate(vTaskRelativeDelay, "RelativeDelay", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);

    /* 创建绝对延时任务，优先级为 1 */
    xTaskCreate(vTaskAbsoluteDelay, "AbsoluteDelay", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
}

/* ========================================================================
 * 6. 栈溢出检测演示
 * ======================================================================== */

static void trigger_recursion(int depth)
{
    /* 每次递归分配 80 字节的局部数组 */
    volatile char local_arr[80];
    local_arr[0] = (char)depth;
    local_arr[79] = (char)depth;
    
    /* 强制触发上下文切换，让调度器在递归过程中检查栈溢出 */
    taskYIELD();
    
    if (depth > 0)
    {
        trigger_recursion(depth - 1);
    }
    
    /* 打印以防止编译器优化掉整个递归 */
    printfSafe("[Recursion] depth=%d, SP addr=0x%x\r\n", depth, (unsigned int)&local_arr[0]);
}

static void vTaskStackOverflowTrigger(void *pvParameters)
{
    (void)pvParameters;
    
    /* 等待 2 秒，让系统启动日志打印完整 */
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    printfSafe("\r\n[OverflowTask] Task started. Stack size: 128 words (512 bytes).\r\n");
    printfSafe("[OverflowTask] Attempting to call deep recursion to overflow stack...\r\n");

    /* 调用递归，每次分配 80 字节，递归 6 层将需要 480 字节以上，必溢出 */
    trigger_recursion(6);

    printfSafe("[OverflowTask] Finished (Should not reach here!)\r\n");
    vTaskDelete(NULL);
}

void start_overflow_demo(void)
{
    printfSafe("\r\n=== Start Stack Overflow Test Demo ===\r\n");
    
    /* 创建一个栈大小只有 128 words 的任务，优先级为 1 */
    xTaskCreate(vTaskStackOverflowTrigger, "OverflowTask", 128, NULL, 1, NULL);
}

/* ========================================================================
 * 7. 内存分配失败检测演示 (Malloc Failed Hook)
 * ======================================================================== */

static void vTaskMallocFailTrigger(void *pvParameters)
{
    (void)pvParameters;
    int count = 0;
    void *ptr;

    /* 等待 2 秒，让系统启动日志输出完毕 */
    vTaskDelay(pdMS_TO_TICKS(2000));

    printfSafe("\r\n[MallocTask] Started. Let's exhaust the FreeRTOS heap...\r\n");

    for (;;)
    {
        uint32_t freeHeap = xPortGetFreeHeapSize();
        printfSafe("[MallocTask] Free Heap: %u bytes. Allocation #%d (attempting 500 bytes)...\r\n", 
                   (unsigned int)freeHeap, ++count);

        /* 每次动态申请 500 字节 */
        ptr = pvPortMalloc(500);

        if (ptr != NULL)
        {
            printfSafe("  -> Success! Block pointer: 0x%x\r\n", (unsigned int)ptr);
        }
        else
        {
            /* 理论上，如果钩子使能，内核在内部 pvPortMalloc 失败时就会调用钩子函数，
               程序直接进入死循环，不会执行到这个 else 分支。 */
            printfSafe("  -> Failed! (Should not reach here if hook is enabled!)\r\n");
            vTaskDelete(NULL);
            return;
        }

        /* 稍作延时，让串口打印完毕 */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void start_malloc_demo(void)
{
    printfSafe("\r\n=== Start Malloc Failed Hook Test Demo ===\r\n");
    
    /* 创建内存分配测试任务，优先级为 1 */
    xTaskCreate(vTaskMallocFailTrigger, "MallocTask", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
}

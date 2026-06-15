#include "demos.h"
#include "bsp_led.h"
#include <stdio.h>

/* ========================================================================
 * 1. 基础互斥量演示 (Exercise 1)
 * ======================================================================== */
static int shared_counter = 0;
static SemaphoreHandle_t xMutex = NULL;

static void vTaskWriter1(void *pvParameters)
{
    int loop;
    (void)pvParameters;

    for (loop = 0; loop < 5; loop++)
    {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        shared_counter++;
        taskYIELD();  /* 模拟被抢占 */
        shared_counter++;
        xSemaphoreGive(xMutex);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    xSemaphoreTake(xMutex, portMAX_DELAY);
    printfSafe("Writer1 done, shared_counter = %d\r\n", shared_counter);
    xSemaphoreGive(xMutex);

    vTaskDelete(NULL);
}

static void vTaskWriter2(void *pvParameters)
{
    int loop;
    (void)pvParameters;

    for (loop = 0; loop < 5; loop++)
    {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        shared_counter++;
        taskYIELD();
        shared_counter++;
        xSemaphoreGive(xMutex);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    xSemaphoreTake(xMutex, portMAX_DELAY);
    printfSafe("Writer2 done, shared_counter = %d\r\n", shared_counter);
    xSemaphoreGive(xMutex);    

    vTaskDelete(NULL);
}

/* ========================================================================
 * 2. 优先级继承演示 (Exercise 2)
 * ======================================================================== */
static SemaphoreHandle_t xMutexPri = NULL;

static void vTaskLowPriority(void *pvParameters)
{
    (void)pvParameters;

    if (xSemaphoreTake(xMutexPri, portMAX_DELAY) == pdTRUE)
    {
        printfSafe("Low task took mutex, priority: %d\r\n", uxTaskPriorityGet(NULL));
        vTaskDelay(pdMS_TO_TICKS(500));  /* 模拟持有锁的操作 */
        printfSafe("Low task giving mutex, priority: %d\r\n", uxTaskPriorityGet(NULL));
        xSemaphoreGive(xMutexPri);
    }
    vTaskDelete(NULL);
}

static void vTaskMediumPriority(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(100));  /* 纯 CPU 运算模拟 */
    }
}

static void vTaskHighPriority(void *pvParameters)
{
    (void)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(50));  /* 确保低优先级先拿锁 */

    if (xSemaphoreTake(xMutexPri, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
        printfSafe("High task got mutex\r\n");
        xSemaphoreGive(xMutexPri);
    }
    else
    {
        printfSafe("High task TIMEOUT - mutex not available\r\n");
    }
    vTaskDelete(NULL);
}

/* ========================================================================
 * 3. 递归互斥量演示 (Exercise 4)
 * ======================================================================== */
static SemaphoreHandle_t xRecMutex = NULL;

static void log_write(const char *str)
{
    printfSafe("%s", str);
}

static void log_write_line(const char *tag, int value)
{
    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);
    char buf[32];
    sprintf(buf, "[%s] %d\r\n", tag, value);
    log_write(buf);
    xSemaphoreGiveRecursive(xRecMutex);
}

static void print_task_chain(const char **names, int depth)
{
    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);
    if (depth > 0) {
        log_write_line(names[depth - 1], depth);  /* 递归获取 */
        print_task_chain(names, depth - 1);       /* 递归调用 */
    }
    xSemaphoreGiveRecursive(xRecMutex);
}

static void vTaskRecursiveDemo(void *pvParameters)
{
    (void)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(200));  /* 等待优先级继承演示跑完 */

    printfSafe("\r\n=== Exercise 4: Recursive Mutex ===\r\n");

    const char *names[] = {"Idle", "LED", "UART", "LCD"};
    int count = sizeof(names) / sizeof(names[0]);

    print_task_chain(names, count);

    printfSafe("\r\n--- Recursive count test ---\r\n");
    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);
    printfSafe("  Take 1 (count=1)\r\n");
    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);
    printfSafe("  Take 2 (count=2)\r\n");
    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);
    printfSafe("  Take 3 (count=3)\r\n");

    xSemaphoreGiveRecursive(xRecMutex);
    printfSafe("  Give 1 (count=2)\r\n");
    xSemaphoreGiveRecursive(xRecMutex);
    printfSafe("  Give 2 (count=1)\r\n");
    xSemaphoreGiveRecursive(xRecMutex);
    printfSafe("  Give 3 (count=0, released)\r\n");

    printfSafe("=== Recursive Demo Done ===\r\n");
    vTaskDelete(NULL);
}

/* ========================================================================
 * 4. 临界区 vs Mutex vs 二值信号量对比 (Exercise 5 + 7.3)
 * ======================================================================== */
#define TEST_LOOPS 5000
static volatile uint32_t g_ulCounter = 0;
static volatile int g_nCompleted = 0;
static SemaphoreHandle_t xCounterMutex = NULL;
static SemaphoreHandle_t xBinaryProtSem = NULL;

static void vWorkerNoProt(void *pvParams)
{
    int i;
    (void)pvParams;
    for (i = 0; i < TEST_LOOPS; i++) {
        g_ulCounter++;
        taskYIELD();
    }
    g_nCompleted++;
    vTaskDelete(NULL);
}

static void vWorkerMutex(void *pvParams)
{
    int i;
    (void)pvParams;
    for (i = 0; i < TEST_LOOPS; i++) {
        xSemaphoreTake(xCounterMutex, portMAX_DELAY);
        g_ulCounter++;
        taskYIELD();
        xSemaphoreGive(xCounterMutex);
    }
    g_nCompleted++;
    vTaskDelete(NULL);
}

static void vWorkerCritical(void *pvParams)
{
    int i;
    (void)pvParams;
    for (i = 0; i < TEST_LOOPS; i++) {
        taskENTER_CRITICAL();
        g_ulCounter++;
        taskEXIT_CRITICAL();
    }
    g_nCompleted++;
    vTaskDelete(NULL);
}

static void vWorkerBinary(void *pvParams)
{
    int i;
    (void)pvParams;
    for (i = 0; i < TEST_LOOPS; i++) {
        xSemaphoreTake(xBinaryProtSem, portMAX_DELAY);
        g_ulCounter++;
        taskYIELD();
        xSemaphoreGive(xBinaryProtSem);
    }
    g_nCompleted++;
    vTaskDelete(NULL);
}

static void vTestAll(void *pvParams)
{
    (void)pvParams;
    vTaskDelay(pdMS_TO_TICKS(300));  /* 等其他演示跑完 */

    printfSafe("\r\n=== Exercise 5 + 7.3: NoProt / Mutex / Critical / BinSem ===\r\n");

    /* Phase 1: 无保护 */
    g_ulCounter = 0;
    g_nCompleted = 0;
    xTaskCreate(vWorkerNoProt, "WNoP1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vWorkerNoProt, "WNoP2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    while (g_nCompleted < 2) vTaskDelay(1);
    printfSafe("[NoProt]    %lu (expected %d)\r\n", g_ulCounter, TEST_LOOPS * 2);

    /* Phase 2: Mutex 保护 */
    g_ulCounter = 0;
    g_nCompleted = 0;
    xTaskCreate(vWorkerMutex, "WMtx1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vWorkerMutex, "WMtx2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    while (g_nCompleted < 2) vTaskDelay(1);
    printfSafe("[Mutex]     %lu (expected %d)\r\n", g_ulCounter, TEST_LOOPS * 2);

    /* Phase 3: 临界区保护 */
    g_ulCounter = 0;
    g_nCompleted = 0;
    xTaskCreate(vWorkerCritical, "WCri1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vWorkerCritical, "WCri2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    while (g_nCompleted < 2) vTaskDelay(1);
    printfSafe("[Critical]  %lu (expected %d)\r\n", g_ulCounter, TEST_LOOPS * 2);

    /* Phase 4: 二值信号量保护 */
    g_ulCounter = 0;
    g_nCompleted = 0;
    xTaskCreate(vWorkerBinary, "WBin1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vWorkerBinary, "WBin2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    while (g_nCompleted < 2) vTaskDelay(1);
    printfSafe("[BinSem]    %lu (expected %d)\r\n", g_ulCounter, TEST_LOOPS * 2);

    /* 临界区嵌套演示 */
    printfSafe("\r\n--- Nesting test ---\r\n");
    taskENTER_CRITICAL();
    printfSafe("  Outer enter\r\n");
    taskENTER_CRITICAL();
    printfSafe("  Inner enter (count=2)\r\n");
    taskEXIT_CRITICAL();
    printfSafe("  Inner exit (count=1)\r\n");
    taskEXIT_CRITICAL();
    printfSafe("  Outer exit (count=0)\r\n");

    printfSafe("\r\n=== 7.3 Conclusion: BinSem can also protect shared resources at the same priority ===\r\n");
    vTaskDelete(NULL);
}

/* ========================================================================
 * 外部启动接口
 * ======================================================================== */
void start_mutex_demos(void)
{
    /* 1. 基础互斥量 (默认屏蔽，如需测试请取消注释) */
    xMutex = xSemaphoreCreateMutex();
    if (xMutex != NULL) {
        // xTaskCreate(vTaskWriter1, "Writer1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
        // xTaskCreate(vTaskWriter2, "Writer2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    }

    /* 2. 优先级继承演示 (默认屏蔽，如需测试请取消注释) */
    // xMutexPri = xSemaphoreCreateMutex();
    // if (xMutexPri != NULL) {
    //     xTaskCreate(vTaskHighPriority, "HighMtx", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    //     xTaskCreate(vTaskMediumPriority, "MedMtx", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    //     xTaskCreate(vTaskLowPriority, "LowMtx", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    // }

    /* 3. 递归互斥量演示 (已启用) */
    xRecMutex = xSemaphoreCreateRecursiveMutex();
    if (xRecMutex != NULL) {
        xTaskCreate(vTaskRecursiveDemo, "RecDemo", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
    }

    /* 4. 临界区 vs Mutex vs 二值信号量对比 */
    xCounterMutex = xSemaphoreCreateMutex();
    xBinaryProtSem = xSemaphoreCreateBinary();
    if (xBinaryProtSem != NULL) {
        xSemaphoreGive(xBinaryProtSem);
    }

    if (xCounterMutex != NULL && xBinaryProtSem != NULL) {
        xTaskCreate(vTestAll, "TestAll", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
    }
}

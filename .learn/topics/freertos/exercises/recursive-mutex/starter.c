/**
 * FreeRTOS 递归互斥量练习
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "demos.h"

static SemaphoreHandle_t xRecMutex = NULL;
static TaskHandle_t xRecDemoTaskHandle = NULL;
static TaskHandle_t xObserverTaskHandle = NULL;

static void vTaskRecursiveDemo(void *pvParameters);
static void vTaskObserver(void *pvParameters);
static void print_nested_log(int depth);

/**
 * @brief  递归打印函数，模拟深度嵌套的内核调用
 */
static void print_nested_log(int depth)
{
    if (depth <= 0) return;

    /* TODO: 使用递归拿锁 API 获取递归互斥量，设定超时为 portMAX_DELAY */
    // if (xSemaphoreTakeRecursive( ... ) == pdTRUE)
    {
        printfSafe("[Task] Entered depth %d. RecMutex taken.\r\n", depth);
        
        /* 递归调用自身 */
        print_nested_log(depth - 1);

        printfSafe("[Task] Leaving depth %d. Giving RecMutex.\r\n", depth);

        /* TODO: 释放递归互斥量 */
        // xSemaphoreGiveRecursive( ... );
    }
}

static void vTaskRecursiveDemo(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(500));
    printfSafe("\r\n=== Start Recursive Mutex Nesting Test ===\r\n");

    /* 递归深度设为 3 */
    print_nested_log(3);

    printfSafe("=== Nesting Test Finished successfully ===\r\n\r\n");

    vTaskDelete(NULL);
}

static void vTaskObserver(void *pvParameters)
{
    (void)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(100)); // 让 Demo 任务先跑

    printfSafe("[Observer] Trying to take lock...\r\n");
    
    /* 尝试拿锁 */
    if (xSemaphoreTakeRecursive(xRecMutex, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
        printfSafe("[Observer] Successfully took lock!\r\n");
        xSemaphoreGiveRecursive(xRecMutex);
    }
    else
    {
        printfSafe("[Observer] TIMEOUT! Cannot take lock (is it fully released?)\r\n");
    }

    vTaskDelete(NULL);
}

void start_rec_mutex_demo(void)
{
    /* TODO: 创建递归互斥量 */
    // xRecMutex = ...

    if (xRecMutex != NULL)
    {
        xTaskCreate(vTaskRecursiveDemo, "RecDemo", configMINIMAL_STACK_SIZE * 2, NULL, 2, &xRecDemoTaskHandle);
        
        /* 实验 B 开启：创建观察者任务测试成对释放 */
        // xTaskCreate(vTaskObserver, "Observer", configMINIMAL_STACK_SIZE, NULL, 1, &xObserverTaskHandle);
    }
}

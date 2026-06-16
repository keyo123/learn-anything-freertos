/**
 * FreeRTOS 中断安全 API 练习
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "bsp_led.h"
#include "demos.h"

/* 定义测试用信号量 */
SemaphoreHandle_t xISRTestSem = NULL;

static void vTaskISRHandler(void *pvParameters);

/**
 * @brief  高优先级的中断响应任务
 */
static void vTaskISRHandler(void *pvParameters)
{
    (void)pvParameters;
    
    for (;;)
    {
        /* TODO: 阻塞等待中断释放信号量，设定为无限期等待 */
        // if (xSemaphoreTake( ... ) == pdTRUE)
        {
            LED1_TOGGLE;
            printfSafe("[Handler Task] Unblocked! LED1 toggled at tick %d\r\n", (int)xTaskGetTickCount());
        }
    }
}

/**
 * @brief  启动中断安全 API 演示入口
 */
void start_isr_demo(void)
{
    /* TODO: 创建二值信号量 */
    // xISRTestSem = ...

    if (xISRTestSem != NULL)
    {
        /* 创建高优先级响应任务（优先级为 3） */
        xTaskCreate(vTaskISRHandler, "ISRHandler", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
        
        printfSafe("[System] ISR Sync Demo Init Success!\r\n");
    }
}

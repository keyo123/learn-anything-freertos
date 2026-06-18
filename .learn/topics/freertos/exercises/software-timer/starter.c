/**
 * FreeRTOS 软件定时器练习模板
 */

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "demos.h"

/* 定时器句柄声明 */
static TimerHandle_t xHeartbeatTimer = NULL;
static TimerHandle_t xBacklightTimer = NULL;

/* 任务句柄 */
static TaskHandle_t xBacklightControllerHandle = NULL;

/* 回调函数声明 */
static void vHeartbeatTimerCallback(TimerHandle_t xTimer);
static void vBacklightTimerCallback(TimerHandle_t xTimer);
static void vTaskBacklightController(void *pvParameters);

/**
 * @brief  周期性心跳定时器回调函数
 */
static void vHeartbeatTimerCallback(TimerHandle_t xTimer)
{
    /* 获取定时器 ID （仅作为演示） */
    void *pvTimerID = pvTimerGetTimerID(xTimer);
    (void)pvTimerID;

    /* TODO: 1. 翻转 LED1 并打印心跳 Tick 日志 */
    // LED1_TOGGLE;
    // printfSafe( ... );
}

/**
 * @brief  单次背光定时器回调函数
 */
static void vBacklightTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;

    /* TODO: 2. 模拟背光关闭，打印背光超时日志 */
    // printfSafe( ... );
}

/**
 * @brief  模拟按键活动并控制定时器周期的控制器任务
 */
static void vTaskBacklightController(void *pvParameters)
{
    (void)pvParameters;

    /* 启动定时器 */
    printfSafe("[System] Starting software timers...\r\n");
    
    /* TODO: 3. 启动心跳定时器 (xTimerStart) 和背光定时器 (xTimerStart) */
    // xTimerStart( ... );
    // xTimerStart( ... );

    /* 阶段 1：前 3 秒模拟高频的用户活动，并临时将心跳周期缩短为 200ms */
    printfSafe("[Controller] Active mode: Resetting backlight timer and speeding up heartbeat...\r\n");
    
    /* TODO: 4. 使用 xTimerChangePeriod 动态改变心跳周期为 200ms */
    // xTimerChangePeriod( ... );

    int i;
    for (i = 0; i < 3; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(1200)); /* 每 1.2 秒触发一次按键 */
        
        printfSafe("[Controller] Simulated Key Press! Resetting backlight timer...\r\n");
        /* TODO: 5. 模拟用户活动，重置单次定时器以推迟超时关闭 */
        // xTimerReset( ... );
    }

    /* 阶段 2：停止操作，恢复心跳周期为 1000ms，静静等待背光超时关闭 */
    printfSafe("[Controller] Idle mode: Restoring heartbeat to 1000ms. Waiting for backlight timeout...\r\n");
    
    /* TODO: 6. 恢复心跳定时器周期为 1000ms */
    // xTimerChangePeriod( ... );

    /* 结束任务生命周期 */
    vTaskDelete(NULL);
}

/**
 * @brief  启动软件定时器演示入口
 */
void start_timer_demo(void)
{
    /* TODO: 7. 创建周期性心跳定时器
     * - pcTimerName: "HeartbeatTimer"
     * - xTimerPeriodInTicks: pdMS_TO_TICKS(1000)
     * - uxAutoReload: pdTRUE (周期定时器)
     * - pvTimerID: NULL
     * - pxCallbackFunction: vHeartbeatTimerCallback
     */
    // xHeartbeatTimer = xTimerCreate( ... );

    /* TODO: 8. 创建单次背光定时器
     * - pcTimerName: "BacklightTimer"
     * - xTimerPeriodInTicks: pdMS_TO_TICKS(5000)
     * - uxAutoReload: pdFALSE (单次定时器)
     * - pvTimerID: NULL
     * - pxCallbackFunction: vBacklightTimerCallback
     */
    // xBacklightTimer = xTimerCreate( ... );

    if (xHeartbeatTimer != NULL && xBacklightTimer != NULL)
    {
        /* 创建模拟控制任务 */
        xTaskCreate(vTaskBacklightController, "BacklightCtrl", configMINIMAL_STACK_SIZE * 2, NULL, 2, &xBacklightControllerHandle);
    }
}

/**
 * FreeRTOS 软件定时器参考实现
 */

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "demos.h"
#include "bsp_led.h"

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
    (void)xTimer;

    /* 翻转 LED1 并安全打印心跳 */
    LED1_TOGGLE;
    printfSafe("[Heartbeat] Tick! LED1 Toggled.\r\n");
}

/**
 * @brief  单次背光定时器回调函数
 */
static void vBacklightTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;

    /* 模拟背光关闭 */
    printfSafe("[Backlight] Timeout! Screen Backlight OFF.\r\n");
}

/**
 * @brief  模拟按键活动并控制定时器周期的控制器任务
 */
static void vTaskBacklightController(void *pvParameters)
{
    (void)pvParameters;

    /* 启动定时器 */
    printfSafe("[System] Starting software timers...\r\n");
    xTimerStart(xHeartbeatTimer, portMAX_DELAY);
    xTimerStart(xBacklightTimer, portMAX_DELAY);

    /* 阶段 1：前 3 秒模拟高频的用户活动，并临时将心跳周期缩短为 200ms */
    printfSafe("[Controller] Active mode: Resetting backlight timer and speeding up heartbeat (200ms)...\r\n");
    
    /* 缩短周期至 200ms */
    xTimerChangePeriod(xHeartbeatTimer, pdMS_TO_TICKS(200), portMAX_DELAY);

    int i;
    for (i = 0; i < 3; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(1200)); /* 每 1.2 秒触发一次按键 */
        
        printfSafe("[Controller] Simulated Key Press! Resetting backlight timer...\r\n");
        /* 重置单次定时器以推迟超时关闭 */
        xTimerReset(xBacklightTimer, portMAX_DELAY);
    }

    /* 阶段 2：停止操作，恢复心跳周期为 1000ms，静静等待背光超时关闭 */
    printfSafe("[Controller] Idle mode: Restoring heartbeat to 1000ms. Waiting for backlight timeout (5s)...\r\n");
    xTimerChangePeriod(xHeartbeatTimer, pdMS_TO_TICKS(1000), portMAX_DELAY);

    /* 结束任务生命周期 */
    vTaskDelete(NULL);
}

/**
 * @brief  启动软件定时器演示入口
 */
void start_timer_demo(void)
{
    /* 创建周期性心跳定时器 (Auto-Reload = pdTRUE) */
    xHeartbeatTimer = xTimerCreate(
        "HeartbeatTimer",
        pdMS_TO_TICKS(1000),
        pdTRUE,
        NULL,
        vHeartbeatTimerCallback
    );

    /* 创建单次背光定时器 (Auto-Reload = pdFALSE) */
    xBacklightTimer = xTimerCreate(
        "BacklightTimer",
        pdMS_TO_TICKS(5000),
        pdFALSE,
        NULL,
        vBacklightTimerCallback
    );

    if (xHeartbeatTimer != NULL && xBacklightTimer != NULL)
    {
        /* 创建模拟控制任务 */
        xTaskCreate(
            vTaskBacklightController,
            "BacklightCtrl",
            configMINIMAL_STACK_SIZE * 2,
            NULL,
            2,
            &xBacklightControllerHandle
        );
    }
}

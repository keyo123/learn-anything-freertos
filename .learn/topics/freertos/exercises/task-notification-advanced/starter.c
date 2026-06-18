/**
 * FreeRTOS 高级任务通知练习
 */

#include "FreeRTOS.h"
#include "task.h"
#include "demos.h"

/* 定义警报事件的二进制位 */
#define BIT_TEMP_ALERT  (1 << 0) // 温度异常标志位
#define BIT_HUMID_ALERT (1 << 1) // 湿度异常标志位

static TaskHandle_t xAlertProcessorHandle = NULL;
static TaskHandle_t xSensorAlertHandle = NULL;
static TaskHandle_t xSensorDataHandle = NULL;

static void vTaskSensorAlert(void *pvParameters);
static void vTaskSensorData(void *pvParameters);
static void vTaskAlertProcessor(void *pvParameters);

/**
 * @brief  警报源任务：模拟检测到警报标志，并使用 eSetBits 置位通知标志
 */
static void vTaskSensorAlert(void *pvParameters)
{
    (void)pvParameters;
    
    /* 延时 2 秒让系统初始化稳定 */
    vTaskDelay(pdMS_TO_TICKS(2000));

    printfSafe("\r\n=== Start Advanced Notification Test ===\r\n");

    /* 1. 触发温度警报 (置位 BIT_TEMP_ALERT) */
    printfSafe("[SensorAlert] High Temperature detected! Sending alert bit...\r\n");
    /* TODO: 使用 xTaskNotify 向处理器任务发送通知，动作选择 eSetBits，值选择 BIT_TEMP_ALERT */
    // xTaskNotify( ... );

    vTaskDelay(pdMS_TO_TICKS(2000)); // 等待 2 秒

    /* 2. 触发湿度警报 (置位 BIT_HUMID_ALERT) */
    printfSafe("[SensorAlert] High Humidity detected! Sending alert bit...\r\n");
    /* TODO: 使用 xTaskNotify 向处理器任务发送通知，动作选择 eSetBits，值选择 BIT_HUMID_ALERT */
    // xTaskNotify( ... );

    vTaskDelay(pdMS_TO_TICKS(2000));

    /* 3. 同时触发温湿度警报 (置位 BIT_TEMP_ALERT | BIT_HUMID_ALERT) */
    printfSafe("[SensorAlert] CRITICAL! Temp and Humid abnormal! Sending alert bits...\r\n");
    /* TODO: 发送包含温湿度两个标志位的通知 */
    // xTaskNotify( ... );

    /* 实验结束后，删除警报生成任务 */
    vTaskDelete(NULL);
}

/**
 * @brief  数据源任务：模拟实时传输 ADC 数据值，并使用 eSetValueWithOverwrite 覆写通知值
 */
static void vTaskSensorData(void *pvParameters)
{
    (void)pvParameters;
    uint32_t adcRawValue = 1000;

    for (;;)
    {
        adcRawValue += 123;
        if (adcRawValue > 4095) adcRawValue = 1000;

        printfSafe("[SensorData] Realtime ADC value sampled: %d. Sending to processor...\r\n", (int)adcRawValue);

        /* TODO: 使用 xTaskNotify 向处理器任务发送数值，动作选择 eSetValueWithOverwrite */
        // xTaskNotify( ... );

        /* 每隔 3000 毫秒（3秒）发送一次实时数据 */
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/**
 * @brief  核心处理器任务：阻塞等待通知并解析 32 位通知值
 */
static void vTaskAlertProcessor(void *pvParameters)
{
    (void)pvParameters;
    uint32_t ulNotifiedValue = 0;

    for (;;)
    {
        /* TODO: 阻塞等待通知。
         * - ulBitsToClearOnEntry: 进入时不清除任何位 (设为 0)
         * - ulBitsToClearOnExit: 退出时将通知值的所有 32 位全部清零 (设为 0xFFFFFFFF)
         * - pulNotificationValue: 传入 &ulNotifiedValue 用以保存读到的通知值
         * - xTicksToWait: 设定无限期等待 (portMAX_DELAY)
         */
        // BaseType_t xReturn = xTaskNotifyWait( ... );

        // if (xReturn == pdPASS)
        // {
        //     /* TODO: 判断通知值中是否包含警报位 */
        //     /* 提示: 如果 ulNotifiedValue 的警报位非零，说明这是一条警报事件；否则是一条 ADC 实时数据值 */
        //     if ( ... )
        //     {
        //         printfSafe(">>> [Processor] !!! ALERT RECEIVED !!! StatusBits: 0x%02X\r\n", (unsigned int)ulNotifiedValue);
        //         
        //         /* 检查具体是哪一项报警 */
        //         if (ulNotifiedValue & BIT_TEMP_ALERT)
        //         {
        //             printfSafe("    -> [ALERT] Temperature is dangerously HIGH!\r\n");
        //         }
        //         if (ulNotifiedValue & BIT_HUMID_ALERT)
        //         {
        //             printfSafe("    -> [ALERT] Humidity is dangerously HIGH!\r\n");
        //         }
        //     }
        //     else
        //     {
        //         /* 读出具体的 ADC 实时数值 */
        //         printfSafe(">>> [Processor] Normal Data Received: ADC Raw = %d\r\n", (int)ulNotifiedValue);
        //     }
        // }
    }
}

/**
 * @brief  启动高级任务通知演示的入口函数
 */
void start_notify_adv_demo(void)
{
    /* 1. 首先创建处理器任务，它的优先级为 2（中等） */
    xTaskCreate(vTaskAlertProcessor, "Processor", configMINIMAL_STACK_SIZE * 2, NULL, 2, &xAlertProcessorHandle);

    if (xAlertProcessorHandle != NULL)
    {
        /* 2. 创建发送警报任务，优先级为 1 */
        xTaskCreate(vTaskSensorAlert, "SensorAlert", configMINIMAL_STACK_SIZE * 2, NULL, 1, &xSensorAlertHandle);

        /* 3. 创建发送实时数据任务，优先级为 1 */
        xTaskCreate(vTaskSensorData, "SensorData", configMINIMAL_STACK_SIZE * 2, NULL, 1, &xSensorDataHandle);
        
        printfSafe("[System] Advanced Task Notification Demo Init Success!\r\n");
    }
}

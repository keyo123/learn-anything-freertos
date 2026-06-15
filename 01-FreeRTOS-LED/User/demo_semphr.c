#include "demos.h"
#include "bsp_led.h"
#include <stdio.h>

/* ========== 全局句柄 (在 stm32f10x_it.c 中以 extern 引用) ========== */
SemaphoreHandle_t xBinarySem_Key1 = NULL;
SemaphoreHandle_t xBinarySem_Key2 = NULL;
/* 任务通知句柄，供中断服务程序 stm32f10x_it.c 引用 */
TaskHandle_t xBtnNotifyTaskHandle = NULL;

/* ========== 1. ISR 同步演示 (Exercise 7.1) ========== */
static void vTaskBtnHandler(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        if (xSemaphoreTake(xBinarySem_Key1, portMAX_DELAY) == pdTRUE)
        {
            LED1_TOGGLE;
            printfSafe("[BtnHandler] KEY1 pressed! LED1 toggled\r\n");
        }
    }
}

static void vTaskBtnNotifyHandler(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        /* 等待任务通知（参数 pdTRUE代表退出时将通知值清零，等效于二值信号量） */
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
        {
            LED1_TOGGLE;
            printfSafe("[BtnNotify] KEY1 pressed! Tasknotified, LED1 toggled\r\n");
        }
    }
}


/* ========== 2. 双重信号量 Pipeline 演示 (Exercise 7.2) ========== */
static SemaphoreHandle_t xBinarySem_Stage2 = NULL;

static void vTaskStageA(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        xSemaphoreTake(xBinarySem_Key2, portMAX_DELAY);
        LED2_ON;
        vTaskDelay(pdMS_TO_TICKS(100));  /* 模拟数据处理 */
        xSemaphoreGive(xBinarySem_Stage2);
        printfSafe("[StageA] Processing... LED2 ON\r\n");
    }
}

static void vTaskStageB(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        xSemaphoreTake(xBinarySem_Stage2, portMAX_DELAY);
        LED2_OFF;
        printfSafe("[StageB] Pipeline done! LED2 OFF\r\n");
    }
}

/* ========== 3. 信号量与互斥量优先级继承对比 (Exercise 7.4) ========== */
static SemaphoreHandle_t xMutexPriCmp = NULL;
static SemaphoreHandle_t xBinSemPriCmp = NULL;
static volatile int g_nPriCmpDone = 0;

static void vPriCmpLow(void *pvParams)
{
    SemaphoreHandle_t xSem = (SemaphoreHandle_t)pvParams;
    int i;

    if (xSemaphoreTake(xSem, portMAX_DELAY) == pdTRUE)
    {
        printfSafe("[Low]   took lock, priority=%d\r\n", uxTaskPriorityGet(NULL));
        
        /* 模拟持锁进行 CPU 密集计算（不延时） */
        for (i = 0; i < 500000; i++) {
            __NOP();
        }

        printfSafe("[Low]   releasing lock, priority=%d\r\n", uxTaskPriorityGet(NULL));
        xSemaphoreGive(xSem);
    }

    g_nPriCmpDone++;
    vTaskDelete(NULL);
}

static void vPriCmpMed(void *pvParams)
{
    (void)pvParams;
    int i;
    vTaskDelay(pdMS_TO_TICKS(20));  /* 让 Low 先拿锁 */

    /* 中等优先级 CPU 密集运算，阻止 Low 释放锁（若无优先级继承） */
    for (i = 0; i < 50000000; i++) {
        __NOP();
    }
    vTaskDelete(NULL);
}

static void vPriCmpHigh(void *pvParams)
{
    SemaphoreHandle_t xSem = (SemaphoreHandle_t)pvParams;
    vTaskDelay(pdMS_TO_TICKS(30));  /* 让 Low 拿锁、Med 抢占 */

    printfSafe("[High]  trying to take lock...\r\n");
    if (xSemaphoreTake(xSem, pdMS_TO_TICKS(200)) == pdTRUE)
    {
        printfSafe("[High]  GOT lock!\r\n");
        xSemaphoreGive(xSem);
    }
    else
    {
        printfSafe("[High]  TIMEOUT! lock not available\r\n");
    }

    g_nPriCmpDone++;
    vTaskDelete(NULL);
}

static void vTestPriCmp(void *pvParams)
{
    (void)pvParams;
    vTaskDelay(pdMS_TO_TICKS(100));

    /* ---- Test A: Mutex (有优先级继承) ---- */
    printfSafe("\r\n=== 7.4a: Mutex (Priority Inheritance) ===\r\n");
    xMutexPriCmp = xSemaphoreCreateMutex();
    g_nPriCmpDone = 0;

    xTaskCreate(vPriCmpHigh, "HighMtx", configMINIMAL_STACK_SIZE, xMutexPriCmp, 3, NULL);
    xTaskCreate(vPriCmpMed,  "MedMtx",  configMINIMAL_STACK_SIZE, NULL,          2, NULL);
    xTaskCreate(vPriCmpLow,  "LowMtx",  configMINIMAL_STACK_SIZE, xMutexPriCmp, 1, NULL);

    while (g_nPriCmpDone < 2) vTaskDelay(1);

    printfSafe("[Mutex] Low boosted -> High got lock\r\n");
    if (xMutexPriCmp != NULL) {
        vSemaphoreDelete(xMutexPriCmp);
        xMutexPriCmp = NULL;
    }

    /* ---- Test B: Binary Semaphore (无优先级继承) ---- */
    printfSafe("\r\n=== 7.4b: Binary Semaphore (NO priority inheritance) ===\r\n");
    xBinSemPriCmp = xSemaphoreCreateBinary();
    if (xBinSemPriCmp != NULL) {
        xSemaphoreGive(xBinSemPriCmp);  /* 初始化为可用 */
    }
    g_nPriCmpDone = 0;

    xTaskCreate(vPriCmpHigh, "HighBin", configMINIMAL_STACK_SIZE, xBinSemPriCmp, 3, NULL);
    xTaskCreate(vPriCmpMed,  "MedBin",  configMINIMAL_STACK_SIZE, NULL,          2, NULL);
    xTaskCreate(vPriCmpLow,  "LowBin",  configMINIMAL_STACK_SIZE, xBinSemPriCmp, 1, NULL);

    while (g_nPriCmpDone < 2) vTaskDelay(1);

    printfSafe("[BinSem] Low stayed at pri 1 -> High TIMEOUT?\r\n");
    if (xBinSemPriCmp != NULL) {
        vSemaphoreDelete(xBinSemPriCmp);
        xBinSemPriCmp = NULL;
    }

    printfSafe("\r\n=== 7.4 Conclusion: Mutex's priority inheritance prevents priority inversion ===\r\n");
    vTaskDelete(NULL);
}

static SemaphoreHandle_t xParkingSem = NULL;

static void vTaskCar(void *pvParameters)
{
    int car_id = (int)pvParameters;

    for (;;)
    {
        printfSafe("[Car %d] Waiting for parking space...\r\n", car_id);

        // 尝试驶入停车场（占用一个车位资源）
        if (xSemaphoreTake(xParkingSem, portMAX_DELAY) == pdTRUE)
        {
            printfSafe("[Car %d] [IN] Entered, free spaces: %d\r\n",
                        car_id, (int)uxSemaphoreGetCount(xParkingSem));

            // 模拟停车 1~2 秒（每辆车时长略微不同）
            vTaskDelay(pdMS_TO_TICKS(1000 + (car_id * 200)));

            printfSafe("[Car %d] [OUT] Leaving...\r\n", car_id);

            // 释放车位资源
            xSemaphoreGive(xParkingSem);

            printfSafe("[Car %d] Left, free spaces: %d\r\n",
                        car_id, (int)uxSemaphoreGetCount(xParkingSem));

            // 离开后等待 1.5 秒再尝试重新停车
            vTaskDelay(pdMS_TO_TICKS(1500));
        }
    }
}

/* ========================================================================
 * 外部启动接口
 * ======================================================================== */
void start_semphr_demos(void)
{
    /* 1. KEY1 中断同步二值信号量 */
        // xBinarySem_Key1 = xSemaphoreCreateBinary();
        // if (xBinarySem_Key1 != NULL) {
        //     xTaskCreate(vTaskBtnHandler, "BtnHdlr", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
        // }
        /* 1.5. 新增：基于任务通知的按键中断同步 */
    xTaskCreate(vTaskBtnNotifyHandler, "BtnNotify",configMINIMAL_STACK_SIZE, NULL, 2, &xBtnNotifyTaskHandle);

    /* 2. KEY2 中断流水线演示 */
    xBinarySem_Key2 = xSemaphoreCreateBinary();
    xBinarySem_Stage2 = xSemaphoreCreateBinary();
    if (xBinarySem_Key2 != NULL && xBinarySem_Stage2 != NULL) {
        xTaskCreate(vTaskStageA, "StageA", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
        xTaskCreate(vTaskStageB, "StageB", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    }

    /* 3. 优先级继承对比测试 */
    xTaskCreate(vTestPriCmp, "PriCmp", configMINIMAL_STACK_SIZE * 2, NULL, 4, NULL);

    /* 4. 计数信号量停车场资源控制演示 */
    xParkingSem = xSemaphoreCreateCounting(3, 3); // 最大车位 3，初始车位 3
    if (xParkingSem != NULL) {
        xTaskCreate(vTaskCar, "Car1", configMINIMAL_STACK_SIZE, (void *)1, 1, NULL);
        xTaskCreate(vTaskCar, "Car2", configMINIMAL_STACK_SIZE, (void *)2, 1, NULL);
        xTaskCreate(vTaskCar, "Car3", configMINIMAL_STACK_SIZE, (void *)3, 1, NULL);
        xTaskCreate(vTaskCar, "Car4", configMINIMAL_STACK_SIZE, (void *)4, 1, NULL);
        xTaskCreate(vTaskCar, "Car5", configMINIMAL_STACK_SIZE, (void *)5, 1, NULL);
    }    
}



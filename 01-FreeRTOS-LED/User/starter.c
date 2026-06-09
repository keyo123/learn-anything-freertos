/**
 * FreeRTOS 互斥量 (Mutex) 练习 — 中级
 *
 * 打开 README.md 查看完整练习描述。
 * 补全 TODO 部分，然后在你的 FreeRTOS 项目中编译运行。
 *
 * 注意：此代码需要 FreeRTOS 环境（已在 FreeRTOSConfig.h 中启用
 * configUSE_MUTEXES 和 configUSE_PRIORITY_INHERITANCE）。
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_usart.h"
#include "bsp_exti.h"

/* ========== 共享资源 ========== */
static int shared_counter = 0;  /* 需要互斥量保护的共享变量 */
static SemaphoreHandle_t xMutex = NULL;

/* ========== 练习 1：保护共享资源 ========== */

static void vTaskWriter1(void *pvParameters)
{
    int loop;
    (void)pvParameters;

    for (loop = 0; loop < 5; loop++)
    {
        /* TODO: 获取互斥量保护共享变量的访问 */
        // xSemaphoreTake(...);
        xSemaphoreTake(xMutex, portMAX_DELAY);
        /* 临界区：操作共享变量 */
        shared_counter++;
        taskYIELD();  /* 模拟耗时操作，增加被抢占的可能 */
        shared_counter++;

        /* TODO: 释放互斥量 */
        // xSemaphoreGive(...);
        xSemaphoreGive(xMutex);
        /* 短暂延迟，让其他任务有机会运行 */
        vTaskDelay(pdMS_TO_TICKS(10));
    }

     xSemaphoreTake(xMutex, portMAX_DELAY);
    printf("Writer1 done, shared_counter = %d\r\n", shared_counter);
     xSemaphoreGive(xMutex);

    vTaskDelete(NULL);
}

static void vTaskWriter2(void *pvParameters)
{
    int loop;
    (void)pvParameters;

    for (loop = 0; loop < 5; loop++)
    {
        /* TODO: 获取互斥量保护共享变量的访问 */
        xSemaphoreTake(xMutex, portMAX_DELAY);
        /* 临界区：操作共享变量 */
        shared_counter++;
        taskYIELD();
        shared_counter++;

        /* TODO: 释放互斥量 */
        xSemaphoreGive(xMutex);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

     xSemaphoreTake(xMutex, portMAX_DELAY);
    printf("Writer2 done, shared_counter = %d\r\n", shared_counter);
     xSemaphoreGive(xMutex);    

    vTaskDelete(NULL);
}

/* ========== 练习 2：优先级继承演示 ========== */

static SemaphoreHandle_t xMutexPri = NULL;

static void vTaskLowPriority(void *pvParameters)
{
    (void)pvParameters;

    /* 低优先级任务先获取互斥量 */
    if (xSemaphoreTake(xMutexPri, portMAX_DELAY) == pdTRUE)
    {
        /* TODO: 打印当前优先级 —— 获取互斥量之后 */
        printf("Low task took mutex, priority: %d\r\n",
                     uxTaskPriorityGet(NULL));

        /* 长时间持有互斥量（模拟慢速设备操作） */
        vTaskDelay(pdMS_TO_TICKS(500));

        /* TODO: 再次打印当前优先级 —— 此时高优先级任务可能在等待 */
        printf("Low task giving mutex, priority: %d\r\n",
                     uxTaskPriorityGet(NULL));

        xSemaphoreGive(xMutexPri);
    }

    vTaskDelete(NULL);
}

static void vTaskMediumPriority(void *pvParameters)
{
    (void)pvParameters;

    /* 中等优先级任务做计算密集型工作 */
    for (;;)
    {
        /* 单纯的 CPU 运算，不访问共享资源 */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void vTaskHighPriority(void *pvParameters)
{
    (void)pvParameters;

    /* 延迟确保低优先级任务先获取互斥量 */
    vTaskDelay(pdMS_TO_TICKS(50));

    /* 尝试获取互斥量——此时低优先级任务正持有它 */
    if (xSemaphoreTake(xMutexPri, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
        /* TODO: 打印成功获取互斥量 */
        printf("High task got mutex\r\n");

        xSemaphoreGive(xMutexPri);
    }
    else
    {
        /* TODO: 打印超时未能获取 */
        printf("High task TIMEOUT - mutex not available\r\n");
    }

    vTaskDelete(NULL);
}

/* ========== 练习 3：约束验证 ========== */

/* TODO: 在 ISR 中尝试 Give 互斥量，观察编译期或运行期行为 */
void vExampleISR(void)
{
    /* 这行代码会怎样？ */
    // xSemaphoreGiveFromISR(xMutex, NULL);
}

/* ========== main ========== */

int main(void)
{
    /* 初始化硬件和 FreeRTOS（你的项目已有此部分） */
    // prvSetupHardware();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	/* LED 初始化 */
	LED_GPIO_Config();
	USART_Config();
	/* 按键外部中断初始化 */
	EXTI_Key_Config();

    /* === 练习 1：创建互斥量 === */
    /* TODO: 使用 xSemaphoreCreateMutex() 创建互斥量 */
    xMutex = xSemaphoreCreateMutex();

    if (xMutex != NULL)
    {
        /* TODO: 创建两个写任务，优先级相同（如 tskIDLE_PRIORITY + 1） */
        // xTaskCreate(vTaskWriter1, "Writer1", configMINIMAL_STACK_SIZE,
        //             NULL, tskIDLE_PRIORITY + 1, NULL);
        // xTaskCreate(vTaskWriter2, "Writer2", configMINIMAL_STACK_SIZE,
        //             NULL, tskIDLE_PRIORITY + 1, NULL);
    }

    /* === 练习 2：创建优先级继承演示 === */
    xMutexPri = xSemaphoreCreateMutex();

    if (xMutexPri != NULL)
    {
        /* TODO: 创建三个任务，优先级依次增高 */
        xTaskCreate(vTaskLowPriority,  "Low",  configMINIMAL_STACK_SIZE, NULL, 1, NULL);
        xTaskCreate(vTaskMediumPriority, "Med", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
        xTaskCreate(vTaskHighPriority, "High", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    }

    /* 启动调度器 */
    vTaskStartScheduler();

    /* 正常情况下不会到达这里 */
    for (;;)
    {
        __NOP();
    }

    return 0;
}

/* ========== 思考题 ========== */
/*
 * 1. 如果把互斥量换成 xSemaphoreCreateBinary() + xSemaphoreGive()，
 *    练习 2 的行为会有什么不同？为什么？
 *
 * 2. 在练习 1 中，去掉互斥量后运行多次，shared_counter 的最终值
 *    是否总是相同的？为什么？
 *
 * 3. 什么场景下应该用 Mutex 而不是二值信号量？
 *
 * 4. 什么是"递归互斥量"？什么场景需要它？
 */

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
#include "event_groups.h"

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

/* ========== 练习 4：递归互斥量 ========== */

static SemaphoreHandle_t xRecMutex = NULL;

/*
 * 模拟一个"日志写入"场景：
 *
 * log_write()        — 底层写入（需要锁）
 * log_write_line()   — 格式化 + 换行（也需要锁，内部调用 log_write）
 *
 * 如果 xRecMutex 是普通 Mutex，log_write_line() 第二次 Take 会死锁！
 * 递归互斥量允许同一任务重复 Take，只增加内部计数。
 */

static void log_write(const char *str)
{
    /* 模拟写入硬件（如 UART） */
    printf("%s", str);
}

static void log_write_line(const char *tag, int value)
{
    /* 这个函数自己也拿锁，然后调 log_write */
    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);

    char buf[32];
    sprintf(buf, "[%s] %d\r\n", tag, value);
    log_write(buf);  /* log_write 不需要再拿锁，它假定调用者已持有 */

    xSemaphoreGiveRecursive(xRecMutex);
}

/* 递归遍历打印任务链 — 模拟分层日志 */
static void print_task_chain(const char **names, int depth)
{
    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);

    if (depth > 0) {
        /* 调用同样需要锁的格式化打印 */
        log_write_line(names[depth - 1], depth);  /* ← 递归获取！普通 Mutex 会死锁 */
        print_task_chain(names, depth - 1);        /* 递归调用 */
    }

    xSemaphoreGiveRecursive(xRecMutex);
}

static void vTaskRecursiveDemo(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(200));  /* 等练习 2 跑完 */

    printf("\r\n=== Exercise 4: Recursive Mutex ===\r\n");

    const char *names[] = {"Idle", "LED", "UART", "LCD"};
    int count = sizeof(names) / sizeof(names[0]);

    print_task_chain(names, count);

    /* 演示递归计数：Take N 次，必须 Give N 次 */
    printf("\r\n--- Recursive count test ---\r\n");

    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);
    printf("  Take 1 (count=1)\r\n");

    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);
    printf("  Take 2 (count=2)\r\n");

    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);
    printf("  Take 3 (count=3)\r\n");

    /* 少于 3 次 Give → Mutex 不会真正释放 */
    xSemaphoreGiveRecursive(xRecMutex);
    printf("  Give 1 (count=2)\r\n");
    xSemaphoreGiveRecursive(xRecMutex);
    printf("  Give 2 (count=1)\r\n");
    xSemaphoreGiveRecursive(xRecMutex);
    printf("  Give 3 (count=0, released)\r\n");

    printf("=== Recursive Demo Done ===\r\n");

    vTaskDelete(NULL);
}

/* ========== 练习 5：临界区 vs Mutex ========== */

/*
 * 一个协调任务串行执行三轮测试（每轮创建两个工作任务）+ 嵌套演示。
 * 这样 printf 不会交错，结果清晰可读。
 */

#define TEST_LOOPS 5000

static volatile uint32_t g_ulCounter = 0;
static volatile int g_nCompleted = 0;
static SemaphoreHandle_t xCounterMutex = NULL;

/* 三个工作任务的模式：无保护 / Mutex / 临界区 */

static void vWorkerNoProt(void *pvParams)
{
    int i;
    (void)pvParams;
    for (i = 0; i < TEST_LOOPS; i++) {
        g_ulCounter++;
        taskYIELD();  /* 主动让出 CPU，给另一个任务机会打断 */
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
        taskYIELD();  /* 安全！Mutex 还持有着，别的任务进不来 */
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

/* 协调任务：依次执行三轮 + 嵌套演示 */
static void vTestAll(void *pvParams)
{
    (void)pvParams;

    vTaskDelay(pdMS_TO_TICKS(300));  /* 等练习 2、4 跑完 */

    printf("\r\n=== Exercise 5: Critical Section vs Mutex ===\r\n");

    /* ---- Phase 1: 无保护 ---- */
    g_ulCounter = 0;
    g_nCompleted = 0;
    xTaskCreate(vWorkerNoProt, "WNoP1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vWorkerNoProt, "WNoP2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    while (g_nCompleted < 2) vTaskDelay(1);  /* 等两个工人都完成 */
    printf("[NoProt]    %lu (expected %d)\r\n", g_ulCounter, TEST_LOOPS * 2);

    /* ---- Phase 2: Mutex 保护 ---- */
    g_ulCounter = 0;
    g_nCompleted = 0;
    xTaskCreate(vWorkerMutex, "WMtx1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vWorkerMutex, "WMtx2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    while (g_nCompleted < 2) vTaskDelay(1);
    printf("[Mutex]     %lu (expected %d)\r\n", g_ulCounter, TEST_LOOPS * 2);

    /* ---- Phase 3: 临界区保护 ---- */
    g_ulCounter = 0;
    g_nCompleted = 0;
    xTaskCreate(vWorkerCritical, "WCri1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vWorkerCritical, "WCri2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    while (g_nCompleted < 2) vTaskDelay(1);
    printf("[Critical]  %lu (expected %d)\r\n", g_ulCounter, TEST_LOOPS * 2);

    /* ---- 临界区嵌套演示 ---- */
    printf("\r\n--- Nesting test ---\r\n");
    taskENTER_CRITICAL();
    printf("  Outer enter\r\n");
    taskENTER_CRITICAL();
    printf("  Inner enter (count=2)\r\n");
    taskEXIT_CRITICAL();
    printf("  Inner exit (count=1)\r\n");
    taskEXIT_CRITICAL();
    printf("  Outer exit (count=0)\r\n");

    /* 临界区内不能阻塞（可取消注释放大镜看看）*/
    // taskENTER_CRITICAL();
    // vTaskDelay(pdMS_TO_TICKS(10));  /* ← 死锁！关中断后无 SysTick 切换 */
    // taskEXIT_CRITICAL();

    printf("=== Critical Section Demo Done ===\r\n");

    vTaskDelete(NULL);
}

/* ========== 练习 6：事件组 ========== */

/*
 * 温控采集系统模拟：
 *
 * BIT_0 — 传感器数据就绪（Sensor 每 200ms 采集一次）
 * BIT_1 — 用户确认采集（User 每 300ms 确认一次）
 *
 * Processor 等待 BIT_0 AND BIT_1 → 处理数据 → 自动清位
 *
 * 预期：两个位都置位时 Processor 才被唤醒。
 *       Sensor(200ms) vs User(300ms) 周期不同，所以不是每次采集都触发处理。
 */

#define BIT_SENSOR  (1 << 0)
#define BIT_CONFIRM (1 << 1)

static EventGroupHandle_t xEventGroup = NULL;

static void vTaskSensor(void *pvParams)
{
    (void)pvParams;
    int i;

    for (i = 1; i <= 5; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(200));
        printf("[Sensor]  采集 #%d\r\n", i);
        xEventGroupSetBits(xEventGroup, BIT_SENSOR);
    }

    vTaskDelete(NULL);
}

static void vTaskUser(void *pvParams)
{
    (void)pvParams;
    int i;

    for (i = 1; i <= 5; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(300));
        printf("[User]    确认 #%d\r\n", i);
        xEventGroupSetBits(xEventGroup, BIT_CONFIRM);
    }

    vTaskDelete(NULL);
}

static void vTaskProcessor(void *pvParams)
{
    (void)pvParams;
    int count = 0;

    while (count < 5)
    {
        /* AND 模式：BIT_SENSOR 和 BIT_CONFIRM 都置位才唤醒，之后自动清位 */
        EventBits_t bits = xEventGroupWaitBits(
            xEventGroup,
            BIT_SENSOR | BIT_CONFIRM,  /* 等待的位 */
            pdTRUE,                    /* 满足后自动清除 */
            pdTRUE,                    /* pdTRUE=AND, pdFALSE=OR */
            portMAX_DELAY
        );

        count++;
        printf(">>> Processor: 处理 #%d (bits=0x%02x)\r\n", count, bits);
    }

    printf("\r\n=== Event Group Demo Done ===\r\n");
    vTaskDelete(NULL);
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

    /* === (屏蔽) 练习 2：优先级继承 === */
    // xMutexPri = xSemaphoreCreateMutex();
    // if (xMutexPri != NULL) {
    //     xTaskCreate(vTaskLowPriority,  "Low",  configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    //     xTaskCreate(vTaskMediumPriority, "Med", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    //     xTaskCreate(vTaskHighPriority, "High", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    // }

    /* === (屏蔽) 练习 4：递归互斥量 === */
    // xRecMutex = xSemaphoreCreateRecursiveMutex();
    // if (xRecMutex != NULL) {
    //     xTaskCreate(vTaskRecursiveDemo, "RecDemo", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
    // }

    /* === (屏蔽) 练习 5：临界区 vs Mutex === */
    // xCounterMutex = xSemaphoreCreateMutex();
    // if (xCounterMutex != NULL) {
    //     xTaskCreate(vTestAll, "TestAll", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
    // }

    /* === 练习 6：事件组 === */
    xEventGroup = xEventGroupCreate();

    if (xEventGroup != NULL)
    {
        /* Processor 优先级 2 > Sensor/User 优先级 1，满足条件时立即处理 */
        xTaskCreate(vTaskSensor,    "Sensor",    configMINIMAL_STACK_SIZE, NULL, 1, NULL);
        xTaskCreate(vTaskUser,      "User",      configMINIMAL_STACK_SIZE, NULL, 1, NULL);
        xTaskCreate(vTaskProcessor, "Processor", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
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

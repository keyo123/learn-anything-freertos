/**
 * FreeRTOS 队列 (Queue) 基础练习
 */

#include "FreeRTOS.h"
#include "demos.h"
#include "queue.h"
#include "task.h"

/* 定义传递的消息结构体 */
typedef struct {
  /* TODO: 添加计数变量 ulCount (uint32_t) */
  uint32_t ulCount;
  /* TODO: 添加数据源标识 cSourceID (char) */
  char cSourceID;
} Message_t;

static QueueHandle_t xTestQueue = NULL;
static TaskHandle_t xSenderHandle = NULL;
static TaskHandle_t xReceiverHandle = NULL;
static TaskHandle_t xTesterHandle = NULL;

static void vTaskSender(void *pvParameters);
static void vTaskReceiver(void *pvParameters);
static void vTaskQueueFullTester(void *pvParameters);

/**
 * @brief  发送者任务：周期性发送数据
 */
static void vTaskSender(void *pvParameters) {
  (void)pvParameters;
  Message_t txMsg = {
      .ulCount = 0,
      .cSourceID = 'S' // 'S' 代表 Sender 发送
  };

  for (;;) {
    txMsg.ulCount++;

    printfSafe("[Sender] Preparing to send message #%d...\r\n",
               (int)txMsg.ulCount);

    /* TODO: 发送结构体变量到队尾。超时时间设为 0 (不等待) */
    BaseType_t xReturn = xQueueSend(xTestQueue,&txMsg,0);

    if (xReturn == pdPASS) {
        printfSafe("[Sender] Message #%d sent successfully!\r\n",
        (int)txMsg.ulCount);
    } else {
        printfSafe("[Sender] Failed to send message #%d (Queue Full!)\r\n",
        (int)txMsg.ulCount);
    }

    /* 延时 1000 毫秒 */
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

/**
 * @brief  接收者任务：阻塞等待接收数据
 */
static void vTaskReceiver(void *pvParameters) {
  (void)pvParameters;
  Message_t rxMsg;

  for (;;) {
    /* TODO: 从队列中接收数据。超时时间设为无限期等待 (portMAX_DELAY) */
    BaseType_t xReturn = xQueueReceive(xTestQueue,&rxMsg,portMAX_DELAY);

    if (xReturn == pdTRUE) {
        printfSafe("[Receiver] Received: MsgCount = %d from Sender '%c'\r\n",
                   (int)rxMsg.ulCount, rxMsg.cSourceID);
    }
  }
}

/**
 * @brief  队列满/超时测试任务：强行塞满并引发超时
 */
static void vTaskQueueFullTester(void *pvParameters) {
  (void)pvParameters;
  Message_t testMsg = {
      .ulCount = 999,
      .cSourceID = 'T' // 'T' 代表 Tester
  };
  int i;

  /* 先延时 3.5 秒，让 Sender 和 Receiver 交互几次，并让队列状态稳定下来 */
  vTaskDelay(pdMS_TO_TICKS(3500));

  /* 暂停接收任务，使队列累积数据直到塞满 */
  printfSafe("\r\n[Tester] Suspending Receiver to fill the queue...\r\n");
  vTaskSuspend(xReceiverHandle);

  printfSafe("[Tester] Starting to send 4 messages to the queue (Queue "
             "capacity is 3)...\r\n");
  for (i = 1; i <= 4; i++) {
    /* TODO: 往队列中发送测试数据。超时等待时间设为 200 毫秒 */
    BaseType_t xReturn = xQueueSend(xTestQueue,&testMsg,pdMS_TO_TICKS(200));

    if (xReturn == pdPASS) {
        printfSafe("[Tester] Message %d sent successfully!\r\n", i);
    } else {
        printfSafe("[Tester] Message %d TIMEOUT! (Queue is full as expected)\r\n", i);
    }
  }

  /* 恢复接收任务，清空队列 */
  printfSafe(
      "[Tester] Resuming Receiver to process accumulated messages...\r\n\r\n");
  vTaskResume(xReceiverHandle);

  /* 完成实验后， Tester 任务自我删除 */
  vTaskDelete(NULL);
}

/**
 * @brief  启动队列演示的入口函数
 */
void start_queue_demo(void) {
  /* TODO: 创建队列，容量深度为 3，单个成员大小为 Message_t 结构体字节数 */
  xTestQueue = xQueueCreate(3,sizeof(Message_t));

  if (xTestQueue != NULL) {
    printfSafe("[System] Queue created successfully!\r\n");

    /* 创建发送者任务：优先级 1 */
    xTaskCreate(vTaskSender, "Sender", configMINIMAL_STACK_SIZE * 2, NULL, 1,
                &xSenderHandle);

    /* 创建接收者任务：优先级 2 (比发送者高，会立即抢占接收) */
    xTaskCreate(vTaskReceiver, "Receiver", configMINIMAL_STACK_SIZE * 2, NULL,
                2, &xReceiverHandle);

    /* 创建测试者任务：优先级 3 */
    xTaskCreate(vTaskQueueFullTester, "Tester", configMINIMAL_STACK_SIZE * 2,
                NULL, 3, &xTesterHandle);
  } else {
    printfSafe("[System] Failed to create queue!\r\n");
  }
}

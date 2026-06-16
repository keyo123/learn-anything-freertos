# Practice Session - 2026-06-16

## Concept Practiced
- Concept: 中断安全API
- Difficulty: Intermediate
- Exercise Name: 中断服务程序同步与断言调试

## User's Submitted Code
在 [demo_isr.c](file:///c:/Users/24212/Desktop/za/study/learn-anything/01-FreeRTOS-LED/User/demo_isr.c) 中：
```c
static void vTaskISRHandler(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    /* 阻塞等待中断释放信号量，设定为无限期等待 */
    if (xSemaphoreTake(xISRTestSem, portMAX_DELAY) == pdTRUE) {
      LED1_TOGGLE;
      printfSafe("[Handler Task] Unblocked! LED1 toggled at tick %d\r\n",
                 (int)xTaskGetTickCount());
    }
  }
}

void start_isr_demo(void) {
  /* 创建二值信号量 */
  xISRTestSem = xSemaphoreCreateBinary();

  if (xISRTestSem != NULL) {
    /* 创建高优先级响应任务（优先级为 3） */
    xTaskCreate(vTaskISRHandler, "ISRHandler", configMINIMAL_STACK_SIZE, NULL,
                3, NULL);

    printfSafe("[System] ISR Sync Demo Init Success!\r\n");
  }
}
```

在 [stm32f10x_it.c](file:///c:/Users/24212/Desktop/za/study/learn-anything/01-FreeRTOS-LED/User/stm32f10x_it.c) 中：
```c
        if (xISRTestSem != NULL)
        {
            xSemaphoreGiveFromISR(xISRTestSem, &xHigherPriorityTaskWoken);
        }
        else if (xBtnNotifyTaskHandle != NULL)
        {
            ...
        }
```

## AI Feedback

1. **✅ 中断同步逻辑实现正确**：
   * 成功通过 `xSemaphoreCreateBinary()` 创建了同步信号量。
   * 任务在循环中通过 `xSemaphoreTake(..., portMAX_DELAY)` 实现无限期阻塞挂起，降低 CPU 消耗。
   * 中断服务函数中正确调用了 `xSemaphoreGiveFromISR` 实现无阻塞地给信号量。

2. **🛠️ 优秀的断言（Assert）排查与修复**：
   * 在实验运行期间，通过捕获 `tasks.c` 的第 1762 行与 1665 行断言错误，精准定位出在屏蔽其它 Demos 时由于 `LED1_Task_Handle` 为 `NULL` 进而导致 `xTaskResumeFromISR(NULL)` 违规调用引起的程序挂起。
   * 通过在中断中添加 `else if (LED1_Task_Handle != NULL)` 指针安全校验，成功消除了空指针引发的断言漏洞，增强了中断运行的鲁棒性。

3. **📝 关于中断实时响应（portYIELD_FROM_ISR）**：
   * 实验表明，在 `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)` 生效时，高优先级的 `vTaskISRHandler` 可以在按键中断处理结束的瞬间立即获得 CPU 运行权（tick 保持高实时性同步），这也印证了 `FromISR` API 触发抢占的意义。

## Assessment
- Understanding: Excellent
- Status update: in_progress → mastered
- confidence: 0.4 → 0.8

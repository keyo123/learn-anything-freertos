# FreeRTOS 高级任务通知：替代事件组与数值传输

## 🎯 目标
- 掌握如何使用 `xTaskNotify` 以 `eSetBits` 和 `eSetValueWithOverwrite` 的动作发送数据。
- 掌握如何使用 `xTaskNotifyWait` 阻塞接收通知、提取 32 位值以及执行特定位的清除动作。
- 通过多任务协作场景，深刻体会任务通知机制（无需创建单独内核对象、点对点轻量高效）的架构特点。

---

## 📋 背景
虽然普通的 `xTaskNotifyGive` / `ulTaskNotifyTake` 可以替代二值信号量，但在更复杂的业务中，我们往往需要：
1. **多路事件标志同步**（比如：等 A 警报和 B 警报置位）。
2. **传输一个 32 位的具体数值**（如 ADC 原始数据，类似于深度为 1 的邮箱队列）。

这两个高频功能通常由“事件组 (Event Group)”或“单成员队列”来完成，但在 FreeRTOS 中，我们完全可以**利用任务内置的 32 位通知值**来实现它们，而且**运行速度快 45%，RAM 零开销**。

---

## ✅ 练习要求

### 1. 多路事件标志置位 (eSetBits)
- 编写任务 `vTaskSensorAlert`（优先级 1），模拟异常监测。
- 该任务会周期性醒来，如果检测到温度过高，使用 `xTaskNotify` 以 `eSetBits` 动作将 `BIT_TEMP_ALERT (1<<0)` 写入目标处理任务 `xAlertProcessorHandle`。
- 如果检测到湿度过高，以 `eSetBits` 动作将 `BIT_HUMID_ALERT (1<<1)` 写入同一目标处理任务。

### 2. 数值覆盖式传输 (eSetValueWithOverwrite)
- 编写任务 `vTaskSensorData`（优先级 1），模拟 ADC 数据采样。
- 该任务会周期性醒来（例如每 1 秒），调用 `xTaskNotify` 以 `eSetValueWithOverwrite` 动作将实时的 ADC 数据（例如 `1024`，`2048`）直接覆盖写入同一目标处理任务。

### 3. 处理任务解码与消费 (xTaskNotifyWait)
- 编写任务 `vTaskAlertProcessor`（优先级 2），这是唯一的接收者。
- 在主循环中，调用 `xTaskNotifyWait` 等待通知。
  - 进入等待时清除哪些位？设为 0（不清除）。
  - 退出等待时清除哪些位？设为 `0xFFFFFFFF`（把所有置位的事件位和数值全部清零，重新开始下一轮等待）。
  - 设定阻塞时间为 `portMAX_DELAY`（无限期等待）。
- 一旦醒来，读取传出的通知值 `ulNotifiedValue`：
  - 如果通知值符合 `BIT_TEMP_ALERT`，打印温度警报。
  - 如果通知值符合 `BIT_HUMID_ALERT`，打印湿度警报。
  - 如果通知值不包含警报位，说明它是一条传感器实时数值数据，打印读出的数值（如：`Received ADC value: XXX`）。

---

## 🔧 修改指引
- 修改 `User/demos.h` 声明 `start_notify_adv_demo()`。
- 修改 `User/main.c` 调用 `start_notify_adv_demo()` 并注释掉 `start_queue_demo()`。
- 将 `demo_notify_adv.c` 加入 `EIDE/.eide/eide.yml` 的 USER 文件夹中。

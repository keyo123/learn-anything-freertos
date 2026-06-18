# FreeRTOS 队列 (Queue) 数据通信与超时处理

## 🎯 目标
- 掌握如何使用 `xQueueCreate` 创建一个包含结构体成员的队列。
- 掌握任务间使用 `xQueueSend` 和 `xQueueReceive` 进行安全的数据（值传递）通信。
- 观察并体验**队列满超时**与**队列空超时**的底层阻塞行为，深化对 RTOS 调度器任务挂起/唤醒的理解。

---

## 📋 背景
在单任务程序中，我们通常使用全局变量传递数据。然而在多任务的 FreeRTOS 中，直接使用全局变量会带来资源竞争、前后台不同步、CPU 空转等待等严重缺陷。

**队列 (Queue)** 是 FreeRTOS 推荐的多任务通信基石。它具有以下三大特性：
1. **先进先出 (FIFO)**：数据按顺序流动。
2. **值拷贝 (Copy by Value)**：将数据深拷贝进队列控制块后面的存储区，保证数据生存期安全。
3. **阻塞调度**：当队列为空/满时，读/写任务会自动挂起（不消耗 CPU），并在数据可读/有空位时由内核自动唤醒。

---

## ✅ 练习要求

### 1. 结构体数据传输
- 补全 `User/demo_queue.c` 中的结构体定义 `Message_t`（包含消息计数值 `ulCount` 和发送源标识 `cSourceID`）。
- 补全 `start_queue_demo`，使用 `xQueueCreate` 创建一个容量（深度）为 3，单个成员大小为 `sizeof(Message_t)` 的队列 `xTestQueue`。
- 创建发送者任务 `vTaskSender`（优先级 1）与接收者任务 `vTaskReceiver`（优先级 2）。

### 2. 正常队列收发与读阻塞
- 补全 `vTaskSender` 中的发送逻辑，每 500ms 往队列中 `xQueueSend` 一条消息，超时时间设为 0（不等待）。
- 补全 `vTaskReceiver` 中的接收逻辑，使用 `xQueueReceive` 无限期阻塞等待接收（`portMAX_DELAY`）。
- **观察**：接收任务的运行状态。即使它的优先级比发送任务高，它是否会因为队列为空而自动阻塞让出 CPU？发送任务发送的一瞬间，接收任务是否立刻抢占运行并打印数据？

### 3. 写超时实验（队列满）
- 在 `start_queue_demo` 中创建第三个高优先级任务 `vTaskQueueFullTester`（优先级 3）。
- 该任务在启动后，先延时一段时间，然后使用 `xQueueSend` 强行往该深度为 3 的队列里连续写入第 4 条数据，并设置写等待超时为 200ms。
- **观察**：由于队列已被低优先级的 `vTaskSender` 塞满（此时 Receiver 延时较长未读完或被暂停），`vTaskQueueFullTester` 是否会因为队列已满而进入阻塞？在 200ms 后，该 API 是否正确返回失败（`errQUEUE_FULL` / `pdFAIL`）并打印超时日志？

---

## 🔧 修改指引
- 修改 `User/demos.h` 声明 `start_queue_demo()`。
- 修改 `User/main.c` 调用 `start_queue_demo()` 并注释掉 `start_rec_mutex_demo()`。
- 将 `demo_queue.c` 加入 `EIDE/.eide/eide.yml` 的 USER 文件夹中。

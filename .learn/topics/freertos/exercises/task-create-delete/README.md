# 任务创建与删除 (Task Creation and Deletion)

## 🎯 目标
在 STM32 + FreeRTOS 平台上，编写并运行任务的**动态创建与自我删除**逻辑，并学会如何使用 `uxTaskGetStackHighWaterMark()` 监测任务堆栈使用情况，防止堆栈溢出。

## 📋 背景
在嵌入式开发中，并不是所有的任务都需要从开机一直运行到关机。例如，蓝牙数据发送任务、U盘数据读取任务等。这类任务只在特定事件触发时才临时创建，完成工作后应该自我销毁（删除），以释放有限的 RAM 资源。

## ✅ 练习要求

### 1. 动态创建任务
- 在 `starter.c` 中补全 `vTaskSupervisor` 任务。
- 当接收到某个触发条件（例如定时器计数或模拟按键触发）时，使用 `xTaskCreate` 动态创建一个优先级较低的 `vTaskWorker` 任务。
- `vTaskWorker` 的堆栈大小初设为 `128` 字（Words）。

### 2. 任务业务与堆栈监测
- 在 `vTaskWorker` 任务中进行一些模拟工作（例如闪烁 LED2 若干次，并打印信息）。
- 在 `vTaskWorker` 运行期间，调用 `uxTaskGetStackHighWaterMark()`，通过串口打印当前任务的**历史最小剩余堆栈大小（高水位线）**。

### 3. 任务自我销毁
- 当 `vTaskWorker` 执行完循环后，必须调用 `vTaskDelete(NULL)` 删除自己，释放自身的任务控制块（TCB）和堆栈空间。

---

## 💡 提示

### 堆栈高水位线
`uxTaskGetStackHighWaterMark(NULL)` 返回的是以 **字 (Words)** 为单位的剩余空间（在 32 位 STM32 上，1字 = 4字节）。如果返回值为 0，说明堆栈已发生溢出！如果返回值过大（如接近 128），说明分配的堆栈过多，造成 RAM 浪费。

### 任务删除的清理工作
如果一个任务是用 `xTaskCreate` 动态创建的：
- 它调用 `vTaskDelete(NULL)` 后，内核的 **空闲任务 (Idle Task)** 会负责回收其 TCB 和 Stack 空间。因此，系统中**空闲任务必须有运行的机会**（不能被完全饿死），否则会造成内存泄漏。

---

## 🔧 需要修改和运行的文件
- `c:\Users\24212\Desktop\za\study\learn-anything\01-FreeRTOS-LED\User\starter.c` (我们将为你创建此文件，填补其中的 TODO)
- 在 `c:\Users\24212\Desktop\za\study\learn-anything\01-FreeRTOS-LED\User\main.c` 中注释掉其它的 demos，并调用 `start_task_demo()` 进行测试。
